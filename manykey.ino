/* 
 * ManyKey firmware
 * See README.md
 * See LICENSE (MIT)
*/

#include <Keyboard.h>
#include <EEPROM.h>

#define DEBOUNCE_DELAY 10
#define MAX_CHARS_PER_BUTTON 10
#define SERIAL_BUFFER_LENGTH 20
#define SERIAL_BAUD_RATE 9600
#define EEPROM_INTEGRITY_BYTE 123
#define EEPROM_DATA_START 0x10

#define SERIAL_START_BYTE 0xEE
#define SERIAL_END_BYTE 0xFF
#define SERIAL_READ_COMMAND 0x00
#define SERIAL_WRITE_COMMAND 0x01
#define SERIAL_QUERY_SETTINGS_COMMAND 0x02


/* --------- Button declarations and functions */
/* Edit list of pins and count here */
#define BUTTON_COUNT 3
byte buttonPins[] = {2, 4, 6};

typedef struct {
  bool state;
  bool lastReading;
  bool latched;
  unsigned long lastTime;
  byte chars[MAX_CHARS_PER_BUTTON];
  byte pin;
  byte index;
} button;
button buttons[BUTTON_COUNT];
bool buttonReadings[BUTTON_COUNT];

void initButtons(){
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    buttons[i].state = false;
    buttons[i].lastReading = false;
    buttons[i].latched = false;
    buttons[i].lastTime = 0;
    wipeArray(buttons[i].chars, MAX_CHARS_PER_BUTTON);
    buttons[i].chars[0] = 97 + i;
    buttons[i].pin = buttonPins[i];
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttons[i].index = i;
  }
}

void pressChars(button btn){
  for (byte i = 0; i < MAX_CHARS_PER_BUTTON; i++) {
    Keyboard.press(btn.chars[i]);
  }
}

void releaseChars(button btn){
  for (byte i = 0; i < MAX_CHARS_PER_BUTTON; i++) {
    Keyboard.release(btn.chars[i]);
  }
}

void updateButtons() {
  for (byte i = 0; i < BUTTON_COUNT; i++){
    // update button readings
    buttonReadings[i] = !digitalRead(buttons[i].pin);
    if (buttonReadings[i] != buttons[i].lastReading) {
      buttons[i].lastTime = millis();
    }

    // press/release buttons
    if ((millis() - buttons[i].lastTime) > DEBOUNCE_DELAY){
      buttons[i].state = buttonReadings[i];
      if (buttons[i].state && !buttons[i].latched) {
        pressChars(buttons[i]);
        buttons[i].latched = true;
      } else if (!buttons[i].state && buttons[i].latched) {
        releaseChars(buttons[i]);
        buttons[i].latched = false;
      }
    }

    // update last reading
    buttons[i].lastReading = buttonReadings[i];
  }
}


/* --------------- Serial read/write functions */
byte serialReadBuffer[SERIAL_BUFFER_LENGTH];
byte serialWriteBuffer[SERIAL_BUFFER_LENGTH];
bool dataAvailable = false;

void readSerial(){
  if (Serial.available() && !dataAvailable){
    Serial.readBytes(serialWriteBuffer, SERIAL_BUFFER_LENGTH);
    dataAvailable = true;
  }
}

// not super happy with how this reads, probably an area for refactor
void processSerialBuffer(){
  if (serialWriteBuffer[0] == SERIAL_START_BYTE) { // start byte
    if (serialWriteBuffer[1] == SERIAL_READ_COMMAND) { // command byte
      if (parsedIndexValid(serialWriteBuffer[2])){ // button index byte
        for (byte i = 3; i < SERIAL_BUFFER_LENGTH; i++){
          if (serialWriteBuffer[i] == SERIAL_END_BYTE) { // end byte
            writeSerialSwitchStatus(buttons[serialWriteBuffer[2]], SERIAL_READ_COMMAND);
            break;
          }
        }
      }
    } else if (serialWriteBuffer[1] == SERIAL_WRITE_COMMAND) { // command byte
      if (parsedIndexValid(serialWriteBuffer[2])){ // button index byte
        byte newChars[MAX_CHARS_PER_BUTTON];
        wipeArray(newChars, MAX_CHARS_PER_BUTTON);
        for (byte i = 3; i < SERIAL_BUFFER_LENGTH; i++){
          if (serialWriteBuffer[i] == SERIAL_END_BYTE){ // end byte
            for (byte j = 0; j < MAX_CHARS_PER_BUTTON; j++){ // write new chars
              buttons[serialWriteBuffer[2]].chars[j] = newChars[j];
            }
            saveConfigToEEPROM();
            writeSerialSwitchStatus(buttons[serialWriteBuffer[2]], SERIAL_WRITE_COMMAND);
            break;
          }
          newChars[i - 3] = serialWriteBuffer[i];
        }
      }
    } else if (serialWriteBuffer[1] == SERIAL_QUERY_SETTINGS_COMMAND){
      for (byte i = 2; i < SERIAL_BUFFER_LENGTH; i++){
        if (serialWriteBuffer[i] == SERIAL_END_BYTE){ // end byte
          writeSerialQuery();
          break;
        }
      }
    }
  }
  discardSerialBuffer();
}

void writeSerialSwitchStatus(button btn, byte command){
  wipeArray(serialWriteBuffer, SERIAL_BUFFER_LENGTH);
  serialWriteBuffer[0] = 0xEE;
  serialWriteBuffer[1] = command;
  serialWriteBuffer[2] = btn.index;
  byte i = 0;
  while (i < MAX_CHARS_PER_BUTTON){
    if (btn.chars[i] == 0x00){ break; }
    serialWriteBuffer[i+3] = btn.chars[i];
    i++;
  }
  serialWriteBuffer[i+3] = 0xFF;
  Serial.write(serialWriteBuffer, i+4);
}

void writeSerialQuery(){
  wipeArray(serialWriteBuffer, SERIAL_BUFFER_LENGTH);
  serialWriteBuffer[0] = 0xEE;
  serialWriteBuffer[1] = SERIAL_QUERY_SETTINGS_COMMAND;
  serialWriteBuffer[2] = BUTTON_COUNT;
  serialWriteBuffer[3] = MAX_CHARS_PER_BUTTON;
  serialWriteBuffer[4] = 0xFF;
  Serial.write(serialWriteBuffer, 5);
}

void discardSerialBuffer(){
  wipeArray(serialWriteBuffer, SERIAL_BUFFER_LENGTH);
  dataAvailable = false;
}

bool parsedIndexValid(byte index){
  if (index < BUTTON_COUNT) {
    return true;
  } else {
    return false;
  }
}


/* -------------------------- EEPROM functions */
void wipeEEPROM(){
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

void saveConfigToEEPROM(){
  EEPROM.update(0x00, EEPROM_INTEGRITY_BYTE);
  EEPROM.update(0x01, MAX_CHARS_PER_BUTTON);
  EEPROM.update(0x02, BUTTON_COUNT);
  for (byte i = 0; i < BUTTON_COUNT; i++){
    for (byte j = 0; j < MAX_CHARS_PER_BUTTON; j++){
      int address = EEPROM_DATA_START + (i * MAX_CHARS_PER_BUTTON) + j;
      EEPROM.update(address, buttons[i].chars[j]);
    }
  }
}

void loadConfigFromEEPROM(){
  if ((EEPROM.read(0x00) == EEPROM_INTEGRITY_BYTE) &&
      (EEPROM.read(0x01) == MAX_CHARS_PER_BUTTON) &&
      (EEPROM.read(0x02) == BUTTON_COUNT)) {
    for (byte i = 0; i < BUTTON_COUNT; i++){
      for (byte j = 0; j < MAX_CHARS_PER_BUTTON; j++){
        int address = EEPROM_DATA_START + (i * MAX_CHARS_PER_BUTTON) + j;
        buttons[i].chars[j] = EEPROM.read(address);
      }
    }
  } else {
    wipeEEPROM();
    saveConfigToEEPROM();
  }
}


/* ---------------------------- Misc functions */
void wipeArray(byte *arr, int len){
  for (int i = 0; i < len; i++){
    arr[i] = 0;   
  }
}


/* ---------------------------- Setup and loop */
void setup() {
  Keyboard.begin();
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.setTimeout(10);
  initButtons();
  loadConfigFromEEPROM();
}

void loop() {
  updateButtons();
  
  if (dataAvailable) {
    processSerialBuffer();
  } else {
    readSerial();
  }
}

