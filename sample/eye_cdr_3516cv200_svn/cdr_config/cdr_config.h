
#ifndef CDR_CONFIG_H
#define CDR_CONFIG_H


typedef struct record_mp4info
{
	unsigned char nPicStopFlag;	//÷ÿ∏¥≈ƒ’’±Í÷æ
	int nAssociatedVideoFrameCount;
	char strAVPicName[256];
	
}RECORD_MP4INFO;


typedef struct 
{
	char cdrSoftwareVersion[20];
	char cdrHardwareVersion[20];
	char cdrBuildTime[20];
	char cdrDeviceSN[20];
}cdrSystemInfomation;



typedef struct 
{
	char pathPhoto[20];
	char pathIndexPic[20];
	char pathMp4[20];
	char pathXml[20];
	char pathTrail[20];
	char pathSystemLog[20];
}cdrSystemSDFilePathCfg;	

typedef struct 
{
	int type;
	int mode;
	int rotate;
}cdrvideoRecordCfg;

typedef struct 
{
	int logo;
	int time;
	int speed;
	int engineSpeed;
	int position;
	int personalizedSignature;
	char psString[256];
}cdrOsdCfg;

typedef struct 
{
	int wifiMode;
	char apSsid[40];
	char apPasswd[40];
	char staSsid[40];
	char staPasswd[40];
	int dhcp;
	char ipAddr[20];
	char gateway[20];
	char netmask[20];
	char dns[20];
}cdrNetCfg;

typedef struct cdr_system_config
{
	int nNeedUpdateFlag;//Default 0. 1 need write to cdr_syscfg.xml
	int photoWithVideo;
	int collisionVideoUploadAuto;
	int photoVideoUploadAuto;
	int photoUploadAuto;
	char name[256];
	char password[256];
	int volumeRecordingSensitivity;
	int volume;
	int bootVoice;
	int accelerationSensorSensitivity;
	int graphicCorrect;
	int cdrPowerDown;
	int eDog;
	int bluetooth;
	int fmFrequency;
	int telecontroller;
	char rtspLive[256];
	char rtspRecord[256];
	cdrSystemInfomation sCdrSystemInfo;
	cdrSystemSDFilePathCfg sCdrFilePath;
	cdrvideoRecordCfg sCdrVideoRecord;
	cdrOsdCfg sCdrOsdCfg;
	cdrNetCfg sCdrNetCfg;
}CDR_SYSTEM_CONFIG;



extern CDR_SYSTEM_CONFIG g_cdr_systemconfig;

int cdr_config_init(void);

#endif



