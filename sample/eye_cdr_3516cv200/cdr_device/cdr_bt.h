
#ifndef CDR_BT_H
#define CDR_BT_H

typedef enum
{
    CDRBT_CMD_NULL = 0x00,
        
	CDRBT_CMD_OPEN = 0x01,
	CDRBT_CMD_STOP = 0x02,
	
	CDRBT_CMD_ANSWER = 0x03,
     CDRBT_CMD_PHONE = 0x04,

    CDRBT_CMD_REJECT = 0x05,
    CDRBT_CMD_CUT = 0x06,
	
	CDRBT_CMD_VOLADD = 0x07,
	CDRBT_CMD_VOSUB = 0x08,
   	
}eCDRBT_CMD;

int cdr_bt_init(void);
int cdr_bt_deinit(void);
int cdr_bt_cmd(eCDRBT_CMD type);
int cdr_bt_get_status();

#endif



