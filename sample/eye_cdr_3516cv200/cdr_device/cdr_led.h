
#ifndef CDR_LED_H_
#define CDR_LED_H_

#define LED_BLUE    0x00
#define LED_RED		0x01 //WIFI
#define LED_NUMMAX	0x02

// 1个亮的周期是200ms
// 一个显示周期2s
#define LED_PULSE_TM 200000 
#define LED_CYCLE_CNT 10


#define LED_STATUS_OFF 		0x00 //  	00 0000 0000
#define LED_STATUS_ON  		0x3FF // 	11 1111 1111
#define LED_STATUS_FLICK  	0x01 // 		00 0000 0001
#define LED_STATUS_DFLICK 	0x05 // 		00 0000 0101


//init led
int cdr_led_init(void);

/*
*  The effective value from the enumeration parameter
*  eColor : eLedColor
*  eFlickMode :eBrightMode
*/
int cdr_led_contr(int nLedIndex,int nMode);

#endif



