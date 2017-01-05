
/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#ifndef DEVICE_H
#define DEVICE_H


typedef struct
{
    int year; 
    int month; 
    int day;
    int hour;
    int minute;
    int second;
}DATE_TIME;

typedef struct
{
    double latitude;    //经度
    double longitude;   //纬度
    
    int latitude_Degree;   //度 
    int latitude_Cent;     //分
    int latitude_Second;   //秒
    
    int longitude_Degree;  //度 
    int longitude_Cent;    //分
    int longitude_Second;  //秒
    
    float     speed;       //速度
    float     direction;   //航向
    float     height;      //海拔高度

    int satellite;
    unsigned char NS;
    unsigned char EW;
    DATE_TIME D;
}GPS_INFO;

int gps_rmc_parse(char *line, GPS_INFO *GPS);
//int GPS_GGA_Parse(char *line, GPS_INFO *GPS);
void show_gps(GPS_INFO *GPS);

extern GPS_INFO g_GpsInfo;
#endif



