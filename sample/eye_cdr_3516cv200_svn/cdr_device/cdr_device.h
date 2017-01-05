
#ifndef CDR_DEVICE_H
#define CDR_DEVICE_H

#include "cdr_gps_data_analyze.h"

#define CDR_POWER_ON  1
#define CDR_POWER_OFF 0

unsigned char GetBmaAssoVideFlagValue();

int cdr_device_init(void);
int cdr_get_powerflag(void);
int GetForcedRecordFullName(char *pDst);

#endif



