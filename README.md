# HA5KFU FT-857 rigctl emulator

> Notice: Currently WIP, most of this document is a LIE, or rather, specifications on what features to implement

An Arduino (ESP8266) based remote controller for FT-857D, emulating parts of the rigctl protocol used by gpredict.

## Supported rigctl commands:
- f
- F
- AOS, LOS (extended commands, ignored)
- \_

## Extra feature

The board is connected to the FT-857's TX-GND signal, detecting PTT. On PTT, the board can send a rigctl mute command to a listening GQRX via TCP, muting the receiving radio while transmitting.

The IP and port shall be configured via custom rigctl commands (like user function), and the whole feature can be enabled / disabled that way.

