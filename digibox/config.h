#ifndef _CONFIG_H
#define _CONFIG_H 1

#include <Arduino.h>

/*
###############################
# FT-857D CAT interface setup #
###############################

These constants specify the workings of the CAT interface to the FT-857D radio. This program uses SoftwareSerial, so the pins can be specified any valid way (but keep in mind GPIO boot states and special function!)

An example definition is provided for WeMos D1 mini (clone) board, based on https://lastminuteengineers.com/wemos-d1-mini-pinout-reference/:
- PIN_RX is D2 (LOW at boot, OK to use)
- PIN_TX is D0 (HIGH at boot, shoul not be LOW at boot)

It is possible to wire more than one serial interface to the CAT cable, for example, ours use this rigctld emulator AND an USB-Serial converter. RX lines can be tied together (but a small resistor is advised), TX lines should be diode-NOR-ed together (meaning two diodes from the radio's TX pin to the interfaces GPIOs. The radio has internal pullup.)

CAT speed (baud rate) is configurable via the radio's menu No. 019 "CAT RATE". Valid values are 4800, 9600, 38400, with 4800 being default

Serial timeout is used to return timeout errors on read operations. Value is given in ms.
*/
const auto PIN_RX = D2;
const auto PIN_TX = D0;
const auto RADIO_CAT_SPEED = 9600;

// Radio serial timeout in ms
const auto SERIAL_TIMEOUT = 100;


/*
#################
# Remote muting #
#################


*/
const auto PIN_PTT = D1; 


/*
Include guard - since we are defining strings in the header, a linker conflict would happen if every code unit included them. With this, only the main program will.
*/
#ifdef _CONFIG_WIFI

/*
##############
# WiFi setup #
##############

Enter SSID and password for your wifi network here
You can also reconfigure the port for the rigctld emulator server
*/
const char* SSID="";
const char* PASS = "";
const uint16_t TCP_LISTEN_PORT = 4532;



/*
#################
# Remote muting #
#################

This digbox has an extra feature. A pin (specified here) is wired to the CAT connector's "TX GND" pin. If the digibox senses TX operation, it sends a command to another rigctld server. This can be setup to mute a receiving radio while transmitting. The commands given here are for muting GQRX (the PR for that was merged in version 2.17.6, but multi-user remote control is also required for gpredic operation, merged in 2.17.7)

Remote muting can be enabled or disabled with the consant "ENABLE_REMOTE_MUTE".

If enabled, it will send the two strings to the specified server on PTT status change. It will try connecting every 10s if the connecition is broken.
*/
const bool ENABLE_REMOTE_MUTE = true;

const char* MUTE_RIGCTL_IP = "";
const uint16_t MUTE_RIGCTL_PORT = 7356;
const char* MUTE_RIGCTL_COMMAND = "U MUTE 1";
const char* UNMUTE_RIGCTL_COMMAND = "U MUTE 0";


/*
OTA setup.
*/
const char* OTA_NAME = "esp8566-ft857-digrig";
const char* OTA_PASS = "123456";

#endif

#endif