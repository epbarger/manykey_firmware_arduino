# ManyKey Firmware (Arduino/ATmega32u4)
This is project is an easy way to get a macro keyboard project going very quickly.

This is the first step of the [ManyKey Project](http://www.manykey.org). We're working on a cross-platform desktop application to configure the keys of any project using this firmware, without a serial terminal. Eventually we hope to offer open source hardware designs and guidance to make building projects like this as easy as possible.

## Features
- Supports 10+ characters pressed simultaneously per switch (this can be changed very easily if you need more)
- Supports as many buttons or switches as you have pins available
- Key configuration can be updated via USB serial
- Configuration is stored in EEPROM, so it will persist if your creation loses power

## Basic use
1. Update ```BUTTON_COUNT``` and ```buttonPins``` in the source to indicate how many and which pins you'll be using in your project.
2. Flash the sketch to your Arduino.
3. For each pin you specified, connect a switch between that pin and ground. If the pin does not support ```INPUT_PULLUP```, you'll also need to add a pull-up resistor between the pin and V+.
4. Configure the keys over serial.
5. Enjoy life with your new macro keyboard.

## Keys
You can configure regular ASCII letters/keys, as well as the modifiers [described here](https://www.arduino.cc/en/Reference/KeyboardModifiers).

## Serial protocol
#### Read Keys (0x00)

_Request_

Start | Command | Switch Index | Stop
----- | ------- | ------------ | ----
0xEE  | 0x00    | 0x00         | 0xFF

_Response_

Start | Command | Switch Index | Key 1 .. Key N | Stop
----- | --------| ------------ | -------------- | ----
0xEE  | 0x00    | 0x00         | 0x98, 0x43, etc| 0xFF


#### Write Keys (0x01)

_Request_

Start | Command | Switch Index | Key 1 .. Key N | Stop
----- | ------- | ------------ | -------------- | ----
0xEE  | 0x01    | 0x00         | 0x97, 0x43, etc| 0xFF

_Response_

Start | Command | Switch Index | Key 1 .. Key N | Stop
----- | --------| ------------ | -------------- | ----
0xEE  | 0x01    | 0x00         | 0x97, 0x43, etc| 0xFF

#### Query Settings (0x02)

_Request_

Start | Command | Stop
----- | ------- | ----
0xEE  | 0x02    | 0xFF

_Response_

Start | Command | Switch Count | Max Keys Per Switch | Stop
----- | ------- | ------------ | ------------------- | ----
0xEE  | 0x02    | 0x03         | 0xA0                | 0xFF

## Contributing
If you see an area of improvement for this project, please open an issue or PR! :D