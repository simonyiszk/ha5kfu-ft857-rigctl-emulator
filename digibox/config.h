#ifndef _CONFIG_H
#define _CONFIG_H 1

#include <Arduino.h>

const auto PIN_RX = D2;
const auto PIN_TX = D0;
const auto PIN_PTT = D1; 

const auto RADIO_CAT_SPEED = 9600;

// Radio serial timeout in ms
const auto SERIAL_TIMEOUT = 100;

#ifdef _CONFIG_WIFI

const char* SSID="";
const char* PASS = "";

const uint16_t TCP_LISTEN_PORT = 4532;

const char* MUTE_RIGCTL_IP = "";
const uint16_t MUTE_RIGCTL_PORT = 7356;

const char* MUTE_RIGCTL_COMMAND = "U MUTE 1";
const char* UNMUTE_RIGCTL_COMMAND = "U MUTE 0";

const char* OTA_NAME = "esp8566-ft857-digrig";
const char* OTA_PASS = "123456";

#endif

#endif