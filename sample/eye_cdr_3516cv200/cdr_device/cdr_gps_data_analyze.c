/************************************************************************	
** Filename: 	
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0

	Copyright(C) 2011 e-eye CO.LTD. ShenZhen <www.e-eye.cn>

*************************************************************************/
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <memory.h>


#include <errno.h>
#include <pthread.h>
#include <signal.h>  
#include "type.h"
#include "cdr_gps_data_analyze.h"

double gps_get_locate(double temp);
double gps_get_double_number(char *s);
int gps_get_int_number(char *s);
void UTC2BTC(DATE_TIME *GPS);

//�õ�ָ����ŵĶ���λ��
int gps_get_comma(int num,char *str)
{
    int i = 0;
    int j = 0;
    
    int len=strlen(str);
    
    for(i=0;i<len;i++)
    {
        if(str[i]==',')
        {
             j++;
        }
        if(j==num)
        {
            return i+1;
        }
    }
    return 0;
}

int gps_rmc_parse(char *line, GPS_INFO *GPS)
{
    unsigned char ch = 0;
    unsigned char status = 0;
    unsigned char tmp = 0;    
    float lati_cent_tmp = 0;
    float lati_second_tmp = 0;
    float long_cent_tmp = 0;
    float long_second_tmp = 0;
    float speed_tmp = 0;
    
    char *buf = line;
    
    ch = buf[5];
    status = buf[gps_get_comma(2, buf)];

    if (ch == 'C') //���������ַ���C��($GPRMC)
    {
        if (status == 'A') //���������Ч�������
        {     
            GPS->NS = buf[gps_get_comma(4, buf)];
            GPS->EW = buf[gps_get_comma(6, buf)];

            GPS->latitude = gps_get_double_number(&buf[gps_get_comma(3, buf)]);
            GPS->longitude = gps_get_double_number(&buf[gps_get_comma(5, buf)]);

            GPS->latitude_Degree = (int)GPS->latitude / 100; //����γ��
            lati_cent_tmp = (GPS->latitude - GPS->latitude_Degree * 100);
            GPS->latitude_Cent = (int)lati_cent_tmp;
            lati_second_tmp = (lati_cent_tmp - GPS->latitude_Cent) * 60;
            GPS->latitude_Second = (int)lati_second_tmp;

            GPS->longitude_Degree = (int)GPS->longitude / 100;    //���뾭��
            long_cent_tmp = (GPS->longitude - GPS->longitude_Degree * 100);
            GPS->longitude_Cent = (int)long_cent_tmp; 
            long_second_tmp = (long_cent_tmp - GPS->longitude_Cent) * 60;
            GPS->longitude_Second = (int)long_second_tmp;

            speed_tmp = gps_get_double_number(&buf[gps_get_comma(7, buf)]); //�ٶ�(��λ������/ʱ)
            GPS->speed = speed_tmp * 1.85; //1����=1.85����
            GPS->direction = gps_get_double_number(&buf[gps_get_comma(8, buf)]); //�Ƕ�            

            GPS->D.hour = (buf[7] - '0') * 10 + (buf[8] - '0');        //ʱ��
            GPS->D.minute = (buf[9] - '0') * 10 + (buf[10] - '0');
            GPS->D.second = (buf[11] - '0') * 10 + (buf[12] - '0');
            
            tmp = gps_get_comma(9, buf);
            GPS->D.day = (buf[tmp + 0] - '0') * 10 + (buf[tmp + 1] - '0'); //����
            GPS->D.month = (buf[tmp + 2] - '0') * 10 + (buf[tmp + 3] - '0');
            GPS->D.year = (buf[tmp + 4] - '0') * 10 + (buf[tmp + 5] - '0') + 2000;

            UTC2BTC(&GPS->D);

            //show_gps(GPS);
            return 1;
        }        
    }
    
    return 0;
}


void show_gps(GPS_INFO *GPS)  
{  
#if 0
    printf("\n");
    printf("DATE     : %ld-%02d-%02d \n",GPS->D.year,GPS->D.month,GPS->D.day);  
    printf("TIME     : %02d:%02d:%02d \n",GPS->D.hour,GPS->D.minute,GPS->D.second);
    
    printf("Latitude : %4.4f %c\n",GPS->latitude,GPS->NS);     
    printf("degree:%d,cent:%d,second:%d\n",GPS->latitude_Degree,GPS->latitude_Cent,GPS->latitude_Second);     

    printf("Longitude: %4.4f %c\n",GPS->longitude,GPS->EW);    
    printf("degree:%d,cent:%d,second:%d\n",GPS->longitude_Degree,GPS->longitude_Cent,GPS->longitude_Second);     

    printf("speed     : %4.4f \n",GPS->speed);      
    //printf("STATUS   : %c\n",GPS->status);     
#endif	
} 

//����ȡ�ı���Ϣת��Ϊdouble��
double gps_get_double_number(char *s)
{
    char buf[128];
    int i;
    double rev;
    i=gps_get_comma(1,s);
    strncpy(buf,s,i);
    buf[i]=0;
    rev=atof(buf);

    return rev;
}

int gps_get_int_number(char *s)
{
    char buf[128] = {0};
    int i;
    double rev;
    i=gps_get_comma(1,s);
    strncpy(buf,s,i);
    buf[i]=0;
    rev=atoi(buf);

    return rev;
}

double gps_get_locate(double temp)
{
    int m;
    double  n;
    m=(int)temp/100;
    n=(temp-m*100)/60;
    n=n+m;
    return n;

}


//������ʱת��Ϊ����ʱ
void UTC2BTC(DATE_TIME *GPS)
{
    //***************************************************
    //�������ȳ�,�ٳ�ʱ������,��ʱ������+1��
    GPS->second++; //��һ��
    if(GPS->second>59){
        GPS->second=0;
        GPS->minute++;
        if(GPS->minute>59){
            GPS->minute=0;
            GPS->hour++;
        }
    }

    //***************************************************
    GPS->hour+=8;
    if(GPS->hour>23)
    {
        GPS->hour-=24;
        GPS->day+=1;
        if(GPS->month==2 ||
                GPS->month==4 ||
                GPS->month==6 ||
                GPS->month==9 ||
                GPS->month==11 ){
            if(GPS->day>30){
                GPS->day=1;
                GPS->month++;
            }
        }
        else{
            if(GPS->day>31){
                GPS->day=1;
                GPS->month++;
            }
        }
        if(GPS->year % 4 == 0 ){
            if(GPS->day > 29 && GPS->month ==2){
                GPS->day=1;
                GPS->month++;
            }
        }
        else{
            if(GPS->day>28 && GPS->month ==2){
                GPS->day=1;
                GPS->month++;
            }
        }
        if(GPS->month>12){
            GPS->month-=12;
            GPS->year++;
        }
    }
}

/*
GPRMC������ʹ����СGPS ���ݸ�ʽ��

$GPRMC,082006.000,A,3852.9276,N,11527.4283,E,0.00,0.0,261009,,*38

$GPRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11><CR><LF>

(1) ��׼��λʱ�䣨UTC time����ʽ��ʱʱ�ַ�����.�����루hhmmss.sss����

(2) ��λ״̬��A = ���ݿ��ã�V = ���ݲ����á�

(3) γ�ȣ���ʽ���ȶȷַ�.�ַַַ֣�ddmm.mmmm����

(4) γ�����֣�������N�����ϰ���S����

(5) ���ȣ���ʽ���ȶȷַ�.�ַַַ֡�

(6) �������֣�����E�����������W������

(7) ���λ���ٶȣ� 0.0 ��1851.8 knots

(8) ���λ�Ʒ���000.0 ��359.9 �ȡ�ʵ��ֵ��

(9) ���ڣ���ʽ�������������꣨ddmmyy����

(10) �ż�������000.0 ��180.0��

(11) ������

(12) Checksum.(���λ)

*/



