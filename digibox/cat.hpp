#include <SoftwareSerial.h>

typedef enum {
  STATUS_OK = 0,
  STATUS_PARSE_ERROR = 1,
  STATUS_HW_ERROR = 2,
} RIGCTL_STATUS;

extern SoftwareSerial radioSer;

RIGCTL_STATUS setRadioMode(const char* new_mode);
RIGCTL_STATUS setRadioFreq(uint32_t freq);
RIGCTL_STATUS setRadioPTT(char ptt_mode);

typedef struct {
  uint32_t freq;
  const char* mode;
} FREQ_AND_MODE;

RIGCTL_STATUS readRadioFreqMode(FREQ_AND_MODE *resp);