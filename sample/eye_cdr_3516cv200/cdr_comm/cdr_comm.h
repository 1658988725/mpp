
#ifndef CDR_COMM_H
#define CDR_COMM_H

#define PRINT_ENABLE    1
#if PRINT_ENABLE
    #define DEBUG_PRT(fmt...)  \
        do {                 \
            printf("[%s - %d]: ", __FUNCTION__, __LINE__);\
            printf(fmt);     \
        }while(0)
#else
    #define DEBUG_PRT(fmt...)  \
        do { ; } while(0)
#endif



#ifndef SAFE_CLOSE
#define SAFE_CLOSE(fd)    do {  \
  if (fd >= 0) {                  \
    fclose(fd);                  \
    fd = NULL;                      \
  }                               \
} while (0)
#endif


#define SD_MOUNT_PATH "/mnt/mmc/"
#define CDR_CUTMP4_EX_TIME 5

#define CDR_FW_VERSION "vs.2.1-1"
#define CDR_HW_VERSION "vh.1.0"

#define CDR_AUDIO_BOOTVOICE		0
#define CDR_AUDIO_IMAGECAPUTER 	1
#define CDR_AUDIO_NOSD 			3
#define CDR_AUDIO_UPDATE 		4
#define CDR_AUDIO_AssociatedVideo 		5 //关联视频.
#define CDR_AUDIO_FM 			6 //FM
#define CDR_AUDIO_RESETSYSTEM 	7 //恢复出厂设置
#define CDR_AUDIO_FORMATSD 		8 //格式化SD卡
#define CDR_AUDIO_SETTIMEOK 	9//设置系统时间OK
#define CDR_AUDIO_SYSTEMUPDATE 	10//系统升级
#define CDR_AUDIO_LT8900PAIR 	11//蓝牙按键配对
#define CDR_AUDIO_STARTREC 		12//开始录像
#define CDR_AUDIO_USBOUT 		13//USB out power off.
#define CDR_AUDIO_DIDI 			14//强制录制视频连续hold声音


extern int recspslen;
extern int recppslen;

extern unsigned char recspsdata[64];
extern unsigned char recppsdata[64];


time_t _strtotime(char *pstr);

void cdr_log_get_time(unsigned char * ucTimeTemp);
int set_cdr_start_time(void);
int cdr_system_new(const char * cmd);
void cdr_get_curr_time(unsigned char * ucTimeTemp);

#endif



