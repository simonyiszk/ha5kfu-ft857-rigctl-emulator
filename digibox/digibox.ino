/*
  HA5KFU-DIGRIG-ESP
*/


#include <SoftwareSerial.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
//#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include "config.h"

SoftwareSerial radioSer(PIN_RX, PIN_TX, false);
char* buffer[128];
buffer[127] = '\0';

// TODO : TCP
// WiFiManager wifiManager;
WiFiServer server(port);
WiFiClient client;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RX, INPUT);
  pinMode(PIN_PTT, INPUT_PULLUP);

  Serial.begin(115200);
  radioSer.begin(RADIO_CAT_SPEED);

  delay(500);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // WiFi.mode(WIFI_STA);
  ip = WiFi.localIP();
  Serial.println(ip);
  // wifiManager.autoConnect("HA5KFU_DIGRIG");
  server.begin();


  readFreqTask();
}

// TODO: this is not needed
// Changing TCP port task

void taskTCPEnable(uint16_t num)  // Ez nem fog kelleni
{
  if (num == 0)
    tcp_en = false;
  else
    tcp_en = true;
}




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

void taskPTTDetect() {
  bool is_ptt = !digitalRead(PIN_PTT);
  if (is_ptt) {
    // TODO: send it to remote rigctl instead
    WiFiClient websdr = client.connect(MUTE_RIGCTL_IP, MUTE_RIGCTL_PORT);
    websdr.println();
  }
}

int read_with_wait() {
  while (!radioSer.available())
    ;
  return radioSer.read();
}

void writeFreq(char* freq) {


  if (atoi(freq) < 0 || atoi(freq) > 470000000) {
    Serial.println("Out of range frequence");
    Serial.println("Vesztettem");
    return;
  }
  
  while (radioSer.read() > 0)
    ;
  char buff[3];
  buff[2] = '\0';
 for (int i = 0; i < 4; i++) { // Doesn't work currently 
    buff[0] = freq[2 * i];
    buff[1] = freq[2 * i + 1];
    radioSer.write(atoi(&buff)); 
  }

  radioSer.write(1);  // Opcode for frequency setting : 01
}

const struct {
  int mode;
  const char* name;
} ARR[] = {
  { 0, "LSB" },
  { 1, "USB" },
  { 2, "CW" },
  { 3, "CWR" },
  { 8, "WFM" },
  { 0x88, "FM" },
  { 4, "AM" },
  { 0x0a, "DIG" },
  { 0x0c, "PKT" }
};

void setModeTask(char* a) {
  uint32_t mod = 0x0;
  bool valid_mod = 0;
  int i = 0;
  for (i = 0; i < 8; i++) {
    if (strcmp(a, ARR[i].name)) {
      mod = ARR[i].mode;
      valid_mod = 1;
      break;
    }
  }

  if (valid_mod) {
    while (radioSer.read() > 0)
      ;
    radioSer.write(mod);
    radioSer.write(0);
    radioSer.write(0);
    radioSer.write(0);
    radioSer.write(7);
  } else {
    Serial.println("Unknown modulation or error");
  }
  return;
}

void taskSetPTT(char a) {
  uint cmd = 0;
  switch (a) {
    case '0':
      cmd = 0x88;
      break;
    case '1':
    case '2':
    case '3':
      cmd = 0x08;
      break;
    default:
      Serial.println("incorrect PTT mode");
      return;
  }
  while (radioSer.read() > 0)
    ;
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(cmd);
  return;
}

void processRigCTL(char* param) {
  char codeChar = *(param);
  if (*(param + 1) == 'm') {
    readFreqTask();  // TODO: logoló program működését át kéne nézni
  }


  switch (codeChar) {
    case 'f':
      readFreqTask();
      break;

    case 'F':

      writeFreqTask( (param+2) );
      break;

    case 'M':

      setModeTask( (param + 2) );
      break;

    case 'T':

      taskSetPTT(*(param + 2));
      break;
    default:
      Serial.println("Incorrect rigctl command");
  }
}
void SerialReceive() {
  if (Serial.available()) {
    *(buffer + Serial.available()) = '\0';
    for (int i = 0; i < Serial.available(), i++) {
      *(buffer + i) = Serial.read();
    }

    processRigCTL(buffer);
  }
}

void TCPReceive() {
  *(buffer + client.available()) = '\0';
  for (int i = 0; i < client.available(), i++) {
    *(buffer + i) = client.read();
  }

  processRigCTL(buffer);
}




void readFreqTask() {
  while (radioSer.read() > 0)
    ;
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(3);
  uint32_t freq = 0;

  auto r = read_with_wait();
  freq += (r >> 4) * 100 + (r & 0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  freq += (r >> 4) * 100 + (r & 0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  freq += (r >> 4) * 100 + (r & 0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  freq += (r >> 4) * 100 + (r & 0xf) * 10;

  Serial.print(freq);
  Serial.print("\t");
  if (tcp_en)  // és a kliensnek és csatlakozva kell lennie
  {
    client.print(freq);
    client.print("\t");
  }
  auto mode = read_with_wait();
  switch (mode) {
    case 0:
      Serial.println("LSB");
      if (tcp_en) { client.println("LSB"); }
      break;
    case 1:
      Serial.println("USB");
      if (tcp_en) { client.println("USB"); }
      break;
    case 2:
      Serial.println("CW");
      if (tcp_en) { client.println("CW"); }
      break;
    case 3:
      Serial.println("CWR");
      if (tcp_en) { client.println("CWR"); }
      break;
    case 0x82:
      Serial.println("CW-N");
      if (tcp_en) { client.println("CW-N"); }
      break;
    case 4:
      Serial.println("AM");
      if (tcp_en) { client.println("AM"); }
      break;
    case 6:
      Serial.println("WFM");
      if (tcp_en) { client.println("WFM"); }
      break;
    case 8:
      Serial.println("FM");
      if (tcp_en) { client.println("FM"); }
      break;
    case 0x88:
      Serial.println("NFM");
      if (tcp_en) {
        client.println("NFM");
      }
      break;
    case 0xa:
      Serial.println("DIG");
      if (tcp_en) { client.println("DIG"); }
      break;
    case 0xc:
      Serial.println("PKT");
      if (tcp_en) { client.println("PKT"); }
      break;
    default:
      Serial.println("Unknown mode");
      if (tcp_en) { client.println("Unknown Mode"); }
  }
}

// the loop function runs over and over again forever
void loop() {
  // TODO: cliens connect and reading TCP &&Serial

  if (!client || client.connected()) {
    client = server.available();
  }

  if (client && client.connected() && client.available()) {
    TCPReceive();
  }
  SerialReceive();
}
