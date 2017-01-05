
#ifndef BMA250E_H
#define BMA250E_H

typedef enum{			   	
   BMA250_MODE_CLOSE     = 0x00,           
   BMA250_MODE_LOW       = 0x01,           
   BMA250_MODE_MIDDLE    = 0x02,           
   BMA250_MODE_HIGHT     = 0x03,        
}eBMA250DeviceMode;

//callback fun
typedef int (*cdr_bma250_callback)(unsigned int uiRegValue);

//init bma250
int cdr_init_bma250(unsigned char ucMode);

//set cdr statu mode
void  cdr_bma250_mode_ctrl(eBMA250DeviceMode ucMode);

//Register callback function.
void cdr_bma250_setevent_callbak(cdr_bma250_callback callback);

#endif



