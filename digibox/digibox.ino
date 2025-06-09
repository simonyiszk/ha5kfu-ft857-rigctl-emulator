/*
  HA5KFU-DIGRIG-ESP
*/

const auto PIN_RX = D2;
const auto PIN_TX = D0;
const auto PIN_PTT = D1; 

#include <SoftwareSerial.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>


SoftwareSerial radioSer(PIN_RX, PIN_TX, false);


// TODO : TCP


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RX, INPUT);
  pinMode(PIN_PTT, INPUT_PULLUP);
  Serial.begin(115200);
  radioSer.begin(9600);
  delay(500);
  readFreqTask();
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
  bool is_ptt = ! digitalRead(PIN_PTT);
  if (is_ptt)
  {
      // TODO: TCP packet send to mute 
  }
}

int read_with_wait() {
  while(!radioSer.available());
  return radioSer.read();
}

void writeFreqTask(int freq){
  if (freq < 0 ||freq >470*10^6)
  {
    Serial.println("Out of range frequence");
    Serial.println("Vesztettem");
    return;
  }
  // example freq = 433 123 456    
  while(radioSer.read() > 0);

  radioSer.write(freq/(10^7)); // send: 43
  radioSer.write(freq/(10^5) - ( (freq/(10^7) ) *100) );
  radioSer.write(freq/(10^3) - ( (freq/(10^5) ) *100) );
  radioSer.write(freq/(10^1) - ( (freq/(10^3) ) *100) );
  radioSer.write(1); // Opcode for frequency setting : 01
  
  
}

void setModeTask(String a)
{
  uint32_t mod = 0x0; 
  bool valid_mod = 1;
  if (a == "LSB")
  {
    mod = 0;
  }
  else if (a == "USB")
  {
    mod = 1;
  }
 else if (a == "CW")
  {
    mod = 2;
  }
  else if (a == "CWR")
  {
    mod = 3;
  }
  else if (a == "WFM") // Ez ha jól tudom akkor a jézuson a sima FM
  {
    mod = 8;
  }
  else if (a == "FM") // Ez pedig az FM-N
  {
    mod = 0x88;
  }
  else if (a == "AM")
  {
    mod = 4;
  }
  else if (a == "DIG")
  {
    mod = 0x0a;
  }
  else if (a == "PKT")
  {
    mod = 0x0c;
  }
  else
  {
    valid_mod = 0;
  }
  if (valid_mod)
  {
  while(radioSer.read() > 0);
  radioSer.write(mod);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(7);
  }
  else
  {
    Serial.println("Unknown mode");
  }
  return;
}

void taskSetPTT(int a)
{
  uint cmd = 0;
  switch (a)
  {
    case '0':
      cmd = 0x88; break;
    case '1':
      cmd = 0x08; break;
    case '2':
      cmd = 0x08; break;
    case '3':
      cmd = 0x08; break;
    default:
    Serial.println("incorrect PTT mode");
     return;
  }
  while(radioSer.read() > 0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(cmd);
return;

}


void menuTask()
{
if (Serial.available())
{
char codeChar = Serial.read();

switch (codeChar){
  case 'f':
  readFreqTask(); break;
  
  case 'F': 
  int freq = 0;
  sscanf(Serial.readStringUntil('\n').c_str(), "%d", &freq);
  writeFreqTask(freq);
  break;

  case 'M':
  String r_mode;
  r_mod =Serial.readString();
  r_mod.trim();
  setModeTask(r_mod);
  break;
  case 'T':
  int ptt_m = 0;
  sscanf(Serial.readStringUntil('\n').c_str(), "%d", &ptt_m);
  taskSetPTT(ptt_m);
  break;
  default:
  Serial.println("Incorrect rigctl command");

}
return;
}

}


void readFreqTask() {
  while(radioSer.read() > 0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(3);
  uint32_t freq = 0;

  auto r = read_with_wait();
  freq += (r>>4)*100 + (r&0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  freq += (r>>4)*100 + (r&0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  freq += (r>>4)*100 + (r&0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  freq += (r>>4)*100 + (r&0xf) * 10;

  Serial.print(freq);
  Serial.print("\t");

  auto mode = read_with_wait();
  switch (mode) {
    case 0:
      Serial.println("LSB"); break;
    case 1:
      Serial.println("USB"); break;
    case 2:
      Serial.println("CW"); break;
    case 3:
      Serial.println("CWR"); break;
    case 0x82:
      Serial.println("CW-N"); break;
    case 4:
      Serial.println("AM"); break;
    case 6:
      Serial.println("WFM"); break;
    case 8:
      Serial.println("FM"); break;
    case 0x88:
      Serial.println("NFM"); break;
    case 0xa:
      Serial.println("DIG"); break;
    case 0xc:
      Serial.println("PKT"); break;
    default:
      Serial.println("Unknown mode");
  }

}

// the loop function runs over and over again forever
void loop() {
  taskHeartbeat();
  
}
