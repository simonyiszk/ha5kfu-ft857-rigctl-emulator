#ifndef _CONFIG_H
#define _CONFIG_H 1

#include <Arduino.h>

const auto PIN_RX = D2;
const auto PIN_TX = D0;
const auto PIN_PTT = D1; 

const auto RADIO_CAT_SPEED = 9600;

#ifdef _CONFIG_WIFI

const char* SSID="";
const char* PASS = "";

const uint16_t TCP_LISTEN_PORT = 4532;

const char* MUTE_RIGCTL_IP = "";
const uint16_t MUTE_RIGCTL_PORT = 4532;

const char* OTA_NAME = "esp8566-ft857-digrig";
const char* OTA_PASS = "123456";

#endif

#endif
