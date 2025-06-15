/*
  HA5KFU-DIGRIG-ESP
*/

const auto PIN_RX = D2;
const auto PIN_TX = D0;
const auto PIN_PTT = D1; 
bool tcp_en = false;
uint16_t port = 4532;

#include <SoftwareSerial.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>


SoftwareSerial radioSer(PIN_RX, PIN_TX, false);


// TODO : TCP
WiFiManager wifiManager;
WiFiServer server(port);
WiFiClient client;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_RX, INPUT);
  pinMode(PIN_PTT, INPUT_PULLUP);
  Serial.begin(115200);
  radioSer.begin(9600);
  delay(500);
  WiFi.mode(WIFI_STA);
  wifiManager.autoConnect("HA5KFU_DIGRIG");
  server.begin();


  readFreqTask();
}

// Changing TCP port task
void taskChangePort(uint16_t newPort)
  {
    if (newport == 0 ||newPort = port) return;
    server.stop();
    server = WiFiServer(newPort);
    server.begin();
    port = newPort;

  }

void taskTCPEnable(uint16_t num)
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
  bool is_ptt = ! digitalRead(PIN_PTT);
  if (is_ptt && tcp_en)
  {
    client.println("TCP üzenet amit még nem néztem meg"); // ezen még változtatni fogok
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
    case '2':
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

void processRigCTL(char codeChar, String data)
{
  switch (codeChar){
  case 'f': // A logoló plugin fm-et használ arra majd valamit kitalálok
  readFreqTask(); 
  
  
  break;
  
  case 'F': 
  int freq = data.toInt();
  writeFreqTask(freq);
  break;

  case 'M':
  data.trim();
  setModeTask(data);
  break;

  case 'T':
  int ptt_m = data.toInt();
  taskSetPTT(ptt_m);
  break;
  default:
  Serial.println("Incorrect rigctl command");
}


void menuTask()
{
if (Serial.available())
{
char codeChar = Serial.read();
String data = Serial.readStringUntil('\n');
processRigCTL(codeChar, data);

}
}

void taskTCPHandle(String cmd)
{
  cmd.trim();
  if (cmd.length() == 0) return;
  char codeChar = cmd.charAt(0);
  String data = cmd.substring(1);
  processRigCTL(codeChar, data);
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
  if (tcp_en) // és a kliensnek és csatlakozva kell lennie
    {
      client.print(freq);
      client.print("\t");
    }
  auto mode = read_with_wait();
  switch (mode) {
    case 0:
      Serial.println("LSB"); if(tcp_en) {client.println("LSB");} break;
    case 1:
      Serial.println("USB"); if(tcp_en) {client.println("USB");} break;
    case 2:
      Serial.println("CW"); if(tcp_en) {client.println("CW");} break;
    case 3:
      Serial.println("CWR"); if(tcp_en) {client.println("CWR");} break;
    case 0x82:
      Serial.println("CW-N"); if(tcp_en) {client.println("CW-N");} break;
    case 4:
      Serial.println("AM"); if(tcp_en) {client.println("AM");} break;
    case 6:
      Serial.println("WFM"); if(tcp_en) {client.println("WFM");} break;
    case 8:
      Serial.println("FM"); if(tcp_en) {client.println("FM");} break;
    case 0x88:
      Serial.println("NFM") if(tcp_en) {client.println("NFM");} break;
    case 0xa:
      Serial.println("DIG"); if(tcp_en) {client.println("DIG");} break;
    case 0xc:
      Serial.println("PKT"); if(tcp_en) {client.println("PKT");} break;
    default:
      Serial.println("Unknown mode"); if(tcp_en) {client.println("Unknown Mode");}
  }
  
}

// the loop function runs over and over again forever
void loop() {
  // TODO: cliens connect and reading TCP &&Serial

  if(!client ||client.connected())
    {
      client = server.available();
    }

  if (client && client.connected() && client.available())
  {
    String cmd = client.readStringUntil('\n');
    taskTCPHandle(cmd);
  }

  menuTask();
}
