/* 
 * ManyKey firmware
 * See LICENSE file
*/

#include <Keyboard.h>
#include <EEPROM.h>

#define DEBOUNCE_DELAY 10
#define MAX_CHARS_PER_BUTTON 10
#define SERIAL_BUFFER_LENGTH 20

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
} button;
button buttons[BUTTON_COUNT];
bool buttonReadings[BUTTON_COUNT];

void initButtons(){
  for (int i = 0; i < BUTTON_COUNT; i++) {
    buttons[i].state = false;
    buttons[i].lastReading = false;
    buttons[i].latched = false;
    buttons[i].lastTime = 0;
    wipeArray(buttons[i].chars, MAX_CHARS_PER_BUTTON);
    buttons[i].chars[0] = 'a';
    buttons[i].pin = buttonPins[i];
    pinMode(buttons[i].pin, INPUT_PULLUP);
  }
}

void pressChars(button btn){
  for (int i = 0; i < MAX_CHARS_PER_BUTTON; i++) {
    Keyboard.press(btn.chars[i]);
  }
}

void releaseChars(button btn){
  for (int i = 0; i < MAX_CHARS_PER_BUTTON; i++) {
    Keyboard.release(btn.chars[i]);
  }
}

void updateButtons() {
  for (int i = 0; i < BUTTON_COUNT; i++){
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
/* ------------------------------------------- */


/* --------------- Serial read/write functions */
byte serialBuffer[SERIAL_BUFFER_LENGTH];

/* ------------------------------------------- */

/* Misc functions ---------------------------- */
void wipeArray(byte *arr, int len){
  for (int i = 0; i < len; i++){
    arr[i] = 0;   
  }
}
/* ------------------------------------------- */

void setup() {
  Keyboard.begin();
  Serial.begin(9600);
  wipeArray(serialBuffer, SERIAL_BUFFER_LENGTH);
  initButtons();
}

void loop() {
  updateButtons();
}

