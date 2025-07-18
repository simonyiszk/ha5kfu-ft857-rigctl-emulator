/*
  HA5KFU-DIGRIG-ESP
*/


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#define _CONFIG_WIFI
#include "config.h"
#include "cat.hpp"

char* buffer[128] = {0};

ESP8266WiFiMulti WiFiMulti;
WiFiServer server(TCP_LISTEN_PORT);
WiFiClient client;

void setup_ota() {
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  // ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(OTA_NAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.begin();
}


void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // init inputs
  pinMode(PIN_RX, INPUT);
  if constexpr(ENABLE_REMOTE_MUTE)
    pinMode(PIN_PTT, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  radioSer.begin(RADIO_CAT_SPEED);

  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFiMulti.addAP(SSID, PASS);
  Serial.println("Begin connecting"); 

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  MDNS.begin("ft857-digrig");
  MDNS.addService("rigctld", "tcp", TCP_LISTEN_PORT);
  setup_ota();

  auto ip = WiFi.localIP();
  Serial.println(ip);
  server.begin();
}


// Simple blinking LED heartbeat
void taskHeartbeat() {
  static uint32_t last_update;
  static bool state;
  if (millis() - last_update > 500) {
    state = !state;
    digitalWrite(LED_BUILTIN, state);
    //Serial.println(state ? "ON" : "OFF");
    last_update = millis();
  }
}


/*
Remote muting task. See config.h for configuration
*/
WiFiClient ptt_client;
bool last_ptt_status;
void taskPTTDetect() {
  // last_checked is used to throttle PTT checking, and this way, also for dumb debouncing. We only check every 100ms
  static uint32_t last_checked;
  // last_tried_to_connect is to throttle TCP connecting, so we don't block the whole program by trying to connect to a shutdown server. We try to connect at least 10s after the last try.
  static uint32_t last_tried_to_connect;


  if (millis() - last_checked < 100) return;
  last_checked = millis();

  bool is_ptt = !digitalRead(PIN_PTT);
  if (is_ptt != last_ptt_status) {
    // connect if we are not
    if (!ptt_client.connected()) {
      // only try to connect every 10s or so
      if (millis() - last_tried_to_connect < 10000) return;
      last_tried_to_connect = millis();
      if (!ptt_client.connect(MUTE_RIGCTL_IP, MUTE_RIGCTL_PORT)) return;
    }

    // actually send the messages
    if (is_ptt)
      ptt_client.println(MUTE_RIGCTL_COMMAND);
    else
      ptt_client.println(UNMUTE_RIGCTL_COMMAND);
  }
  last_ptt_status = is_ptt;
}



const auto BUFFER_SIZE = 80;
/*
Run Rigctld command and write result to output_buffer
@param commandline zero terminated line of command entered with leading and trailing whitespace removed. Example "f", "F 1234"
@param output_buffer buffer for returned string, at least BUFFER_SIZE long
*/
void processRigCTL(const char* commandline, char* output_buffer) {
  char codeChar = commandline[0];
  const char* parameter = &commandline[1];
  while (*parameter == ' ' || *parameter=='\t') parameter++;

  FREQ_AND_MODE resp;
  RIGCTL_STATUS status;
  uint32_t new_freq;
  switch (codeChar) {
    case 'f':
  
      status = readRadioFreqMode(&resp);
      if (!status)
        snprintf(output_buffer, BUFFER_SIZE, "%d\n", resp.freq);
      else
        snprintf(output_buffer, BUFFER_SIZE, "RPRT %d\n", status);
      output_buffer[BUFFER_SIZE-1] = 0;
      break;

    case 'F':
      
      status = STATUS_PARSE_ERROR;
      if (sscanf(parameter, "%d", &new_freq) == 1) {
        status = setRadioFreq(new_freq);
      }
      snprintf(output_buffer, BUFFER_SIZE, "RPRT %d\n", status);
      output_buffer[BUFFER_SIZE-1] = 0;
      break;

    case 'm':
      status = readRadioFreqMode(&resp);
      if (!status)
        snprintf(output_buffer, BUFFER_SIZE, "%5s\n", resp.mode);
      else
        snprintf(output_buffer, BUFFER_SIZE, "RPRT %d\n", status);
      output_buffer[BUFFER_SIZE-1] = 0;
      break;
    case 'M':
      status = setRadioMode(parameter);
      snprintf(output_buffer, BUFFER_SIZE, "RPRT %d\n", status);
      break;
    case 'T':
      status = setRadioPTT(parameter[0]);
      snprintf(output_buffer, BUFFER_SIZE, "RPRT %d\n", status);
      break;
    case '_':
      snprintf(output_buffer, BUFFER_SIZE, "Wireless rigctld emulator for Yaesu FT-857D - 2025.07.13. HA5KFU\n");
      break;
    default:
      snprintf(output_buffer, BUFFER_SIZE, "RPRT %d\n", STATUS_PARSE_ERROR);
      Serial.println("Incorrect rigctl command:");
      Serial.print("\tCommand: "); Serial.println(codeChar);
      Serial.print("\tparameter: "); Serial.println(parameter);
  }
}


void check_clients() {
  WiFiClient new_client = server.available();
  if (new_client) {
    client = new_client;
    client.setTimeout(1000);
  }

  if(client.connected()) {
    if (client.available()) {
      char resp[BUFFER_SIZE];
      auto cmd_str = client.readStringUntil('\n');
      Serial.print("Received command line: "); Serial.println(cmd_str);

      // I had a weird bug where I could only use this, not client.readStringUntil('\n').c_str(). I suspect it was auto-freeing the string that way, and thus, invalidate the pointer.
      const char* cmd = cmd_str.c_str();
      processRigCTL(cmd, resp);
      client.print(resp);

      client.read(); // Read ending newline
      client.flush(); // Flush write buffer
    }
  }
}


// the loop function runs over and over again forever
void loop() {
  ArduinoOTA.handle();
  MDNS.update();
  WiFiMulti.run();
  taskHeartbeat();
  check_clients();
  if constexpr(ENABLE_REMOTE_MUTE)
    taskPTTDetect();
}
