
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
    double latitude;    //����
    double longitude;   //γ��
    
    int latitude_Degree;   //�� 
    int latitude_Cent;     //��
    int latitude_Second;   //��
    
    int longitude_Degree;  //�� 
    int longitude_Cent;    //��
    int longitude_Second;  //��
    
    float     speed;       //�ٶ�
    float     direction;   //����
    float     height;      //���θ߶�

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



