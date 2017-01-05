
#ifndef LT8900_H
#define LT8900_H

typedef enum
{			   
	UNDEFINE,
	LT8900_MODE_CLOSE         = 0x00,	      
	LT8900_MODE_OPEN          = 0x01,	      
	LT8900_MATCH_MODE_ENABLE  = 0x02,	      
	LT8900_MATCH_MODE_DISABLE = 0x03,	      
}eLt8900DeviceMode;

//»Øµ÷º¯Êý.
typedef int (*cdr_lt8900_callback)(int iKeyValue);
//init lt8900
int cdr_init_lt8900(void);
//set cdr statu mode
void  cdr_lt8900_mode_ctrl(eLt8900DeviceMode ucMode);
//Register callback function...
void cdr_lt8900_setevent_callbak(cdr_lt8900_callback callback);

void cdr_lt8900_close(void);


#endif



