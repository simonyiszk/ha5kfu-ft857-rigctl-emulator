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
