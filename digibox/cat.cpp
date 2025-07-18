#include "cat.hpp"
#include "config.h"


/*
Consult FT-857D manual section "CAT operation" for the explanation of the FT-857D serial protocol.
*/
enum FT857_CMD {
  CMD_SET_PTT_ON = 0x08,
  CMD_SET_PTT_OFF = 0x88,
  CMD_SET_MODE = 7,
  CMD_SET_FREQ = 1,
  CMD_READ_FREQ = 3,
};

SoftwareSerial radioSer(PIN_RX, PIN_TX, false);

int read_with_wait() {
  auto timeout_ends = millis() + SERIAL_TIMEOUT;
  while (!radioSer.available() && millis() < timeout_ends);
  if (!radioSer.available())
    return -1;
  return radioSer.read();
}


const struct {
  int ft857_mode;
  const char* rigctl_name;
} FT857_RIGCTL_MODE_MAP[] = {
  { 0, "LSB" },
  { 1, "USB" },
  { 2, "CW" },
  { 3, "CWR" },
  { 0x82, "CW-N"}, // unknown to rigctl
  { 6, "WFM"}, // only in readout, readout has FM-WFM-FMN, write only FM or FMN
  { 8, "WFM" },
  { 0x88, "FM" },
  { 4, "AM" },
  { 0x0a, "DIG" },
  { 0x0c, "PKTUSB" }
};

const auto FT857_RIGCTL_MODE_MAP_SIZE = sizeof(FT857_RIGCTL_MODE_MAP)/sizeof(FT857_RIGCTL_MODE_MAP[0]);

RIGCTL_STATUS setRadioMode(const char* new_mode) {
  uint32_t mode = 0x0;
  bool valid_mode = 0;
  for (int i = 0; i < FT857_RIGCTL_MODE_MAP_SIZE; i++) {
    if (!strcmp(FT857_RIGCTL_MODE_MAP[i].rigctl_name, new_mode)) {
      mode = FT857_RIGCTL_MODE_MAP[i].ft857_mode;
      valid_mode = 1;
      break;
    }
  }

  if (valid_mode) {
    while (radioSer.read() >= 0)
      ;
    radioSer.write(mode);
    radioSer.write(0);
    radioSer.write(0);
    radioSer.write(0);
    radioSer.write(CMD_SET_MODE);
  } else {
    return STATUS_PARSE_ERROR;
  }
  return STATUS_OK;
}

RIGCTL_STATUS setRadioFreq(uint32_t freq) {
  while (radioSer.read()>=0);

  char freq_bcd_rev[8];

  freq /= 10;
  for (int i = 0; i<8; i++) {
    freq_bcd_rev[i] = freq % 10;
    freq /= 10;
  }

  for (int i = 0; i<4; i++) {
    radioSer.write((freq_bcd_rev[7-2*i]<<4) | (freq_bcd_rev[6-2*i]));
  }

  radioSer.write(CMD_SET_FREQ);  // Opcode for frequency setting : 01
  return STATUS_OK;
}



RIGCTL_STATUS setRadioPTT(char ptt_mode) {
  uint cmd = 0;
  switch (ptt_mode - '0') {
    case 0:
      cmd = CMD_SET_PTT_OFF;
      break;
    case 1:
    case 2:
    case 3:
      cmd = CMD_SET_PTT_ON;
      break;
    default:
      return STATUS_PARSE_ERROR;
  }

  while (radioSer.read() > 0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(cmd);
  return STATUS_OK;
}



RIGCTL_STATUS readRadioFreqMode(FREQ_AND_MODE *resp) {
  while (radioSer.read() > 0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(0);
  radioSer.write(CMD_READ_FREQ);

  /*
  Respones is 4 bytes of BCD coded freq
  + 1 byte of mode
  */
  
  uint32_t freq = 0;
  auto r = read_with_wait();
  if (r<0)
    return STATUS_HW_ERROR;
  freq += (r >> 4) * 100 + (r & 0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  if (r<0)
    return STATUS_HW_ERROR;
  freq += (r >> 4) * 100 + (r & 0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  if (r<0)
    return STATUS_HW_ERROR;
  freq += (r >> 4) * 100 + (r & 0xf) * 10;
  freq *= 100;

  r = read_with_wait();
  if (r<0)
    return STATUS_HW_ERROR;
  freq += (r >> 4) * 100 + (r & 0xf) * 10;

  resp->freq = freq;

  // parse mode
  r = read_with_wait();
  if (r<0)
    return STATUS_HW_ERROR;

  resp->mode = "UNKNOWN";
  for (int i = 0; i < FT857_RIGCTL_MODE_MAP_SIZE; i++) {
    if (FT857_RIGCTL_MODE_MAP[i].ft857_mode == r) {
      resp->mode = FT857_RIGCTL_MODE_MAP[i].rigctl_name;
      break;
    }
  }

  return STATUS_OK;
}
