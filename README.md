# HA5KFU FT-857 rigctl emulator

An Arduino (ESP8266) based remote controller for FT-857D, emulating parts of the rigctl protocol used by gpredict.

This firmware is used by our advanced digbox for the FT-857D, used for satelite operation. The digrig has the usual parts like USB hub, USB soundcard, USB-Serial for CAT, but we added the option to power it via the radio's 13V and also wired a WeMos D1 Mini (clone) board to the CAT pins (using diodes on the TX pins), and using this rigctl emulator, the radio can be remote controlled via wifi. 

## Supported rigctl commands:
- `f` (get frequency)
- `F` (set frequency)
- `m` (get mode)
- `M` (set mode)
- `T` (set PTT)
- `\_` (get emulator version)

## Extra feature

The board is connected to the FT-857's TX-GND signal, detecting PTT. On PTT, the board can send a rigctl mute command to a listening GQRX via TCP, muting the receiving radio while transmitting.

This requires GQRX version 2.17.7 to work properly!

## Setup

Wire and connect digrig, configure using `config.h`, build and upload. Assign a static IP to the board in the network, and setup it as an interface in GPredict.

> To use the remote muting feature, you need to allowlist the IP of the board in GQRX's remote configuration tab