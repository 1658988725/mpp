#include "cdr_XmlLog.h"
#include "minxml/mxml.h"
#include <assert.h>

#include "cdr_config.h"

CDR_SYSTEM_CONFIG g_cdr_systemconfig;

#define CDR_DF_WIFI_SSID "KAKA_00000"
#define CDR_DF_WIFI_PWD  "12345678"

//set cdr_syscfg.xml ssid and passwd to init_wifi_7601.sh
int cdr_set_wifi_config(char* ssid,char *pwd)
{
	char cCmd[150] = {0};
	memset(cCmd,0x00,sizeof(cCmd));
	sprintf(cCmd,"sed -i '/EncrypType/ aiwpriv wlan0 set SSID=\"%s\"' /home/wifi_bin/init_wifi_7601.sh",ssid);
	cdr_system("sed -i '/SSID/d' /home/wifi_bin/init_wifi_7601.sh");
	cdr_system(cCmd);
	
	memset(cCmd,0x00,sizeof(cCmd));
	sprintf(cCmd,"sed -i '/SSID/ aiwpriv wlan0 set WPAPSK=\"%s\"' /home/wifi_bin/init_wifi_7601.sh",pwd);
	cdr_system("sed -i '/WPAPSK/d' /home/wifi_bin/init_wifi_7601.sh");
	cdr_system(cCmd);
	cdr_system("sync");
	
	return 0;
}

//reset wifi ssid to KAKA_MAC[5] init_wifi_7601.sh and cdr_syscfg.xml
int cdr_wifi_config_reset()
{
	char chWifiName[20];
	char chMacInfo[13];

	memset(chMacInfo,0x00,sizeof(chMacInfo));
	memset(chWifiName,0x00,sizeof(chWifiName));
	if(cdr_getmac(chMacInfo) != 0)		
		return -1;

	sprintf(chWifiName,"KAKA_%s",chMacInfo);

	cdr_set_wifi_config(chWifiName,CDR_DF_WIFI_PWD);

	strcpy(g_cdr_systemconfig.sCdrNetCfg.apSsid,chWifiName);
	strcpy(g_cdr_systemconfig.sCdrNetCfg.apPasswd,CDR_DF_WIFI_PWD);
		
	save_cfg_to_xml("net","apSsid",chWifiName,1);
	save_cfg_to_xml("net","apPasswd",CDR_DF_WIFI_PWD,1);	
	return 0;
}

int cdr_init_wifi_config()
{
	char ssid[40],pwd[40];
	
	if(g_cdr_systemconfig.sCdrNetCfg.wifiMode == 0)
	{
		strcpy(ssid,g_cdr_systemconfig.sCdrNetCfg.apSsid);
		strcpy(pwd,g_cdr_systemconfig.sCdrNetCfg.apPasswd);
		
		//if ssid == KAKA_00000 
		//update init_wifi_7601.sh ssid to KAKA_MAC[5]
		//update cdr_syscfg.xml ssid to KAKA_MAC[5]
		if(strcmp(CDR_DF_WIFI_SSID,ssid) == 0)
		{			
			cdr_wifi_config_reset();
		}
		else
		{
			//Update cdr_syscfg.xml wifi config to init_wifi_7601.sh
			cdr_set_wifi_config(ssid,pwd);
		}
		cdr_system("sync");
	}	
	return 0;
}

int cdr_get_item_value_str(mxml_node_t *root,char *name,char *sValue)
{
	mxml_node_t *node = NULL;
	if(root == NULL || name == NULL || sValue == NULL)
	{
		printf("%s param is null..\r\n",__FUNCTION__);
		return -1;
	}
	node = mxmlFindElement(root, root, name,NULL, NULL,MXML_DESCEND);
	if(node)
	{
		char *pText = NULL;
		pText = mxmlGetText(node,0);
		strcpy(sValue,pText);	
		return 0;
	}
	//printf("%s = %s \n",name,sValue);
	return -1;	
}
int cdr_get_item_value_int(mxml_node_t *root,char *name)
{	
	char chData[10];
	cdr_get_item_value_str(root,name,chData);
	return atoi(chData);	
}

int cdr_read_xml_to_cfg(void)
{
	FILE *fp;
	//int nRet = -1;
	mxml_node_t *tree = NULL;
	mxml_node_t *node;
	fp = fopen("/home/cdr_syscfg.xml", "r");
	tree = mxmlLoadFile(NULL, fp,MXML_TEXT_CALLBACK);
	fclose(fp);

	CDR_SYSTEM_CONFIG sSystemconfig;
	memset(&sSystemconfig,0x00,sizeof(CDR_SYSTEM_CONFIG));
	
	node = mxmlFindElement(tree, tree, "cdrSystemCfg",NULL, NULL,MXML_DESCEND);
	if(node == NULL)
	{
		printf("cdrSystemCfg node not found..\r\n");
		mxmlRelease(tree);		
		return -1;
	}
		
	sSystemconfig.photoWithVideo = cdr_get_item_value_int(node,"photoWithVideo");    
	sSystemconfig.collisionVideoUploadAuto = cdr_get_item_value_int(node,"collisionVideoUploadAuto");
	sSystemconfig.photoVideoUploadAuto = cdr_get_item_value_int(node,"photoVideoUploadAuto");

	cdr_get_item_value_str(node,"name",sSystemconfig.name);
	cdr_get_item_value_str(node,"password",sSystemconfig.password);


	sSystemconfig.volumeRecordingSensitivity = cdr_get_item_value_int(node,"volumeRecordingSensitivity");
	sSystemconfig.volume = cdr_get_item_value_int(node,"volume");
	sSystemconfig.bootVoice = cdr_get_item_value_int(node,"bootVoice");
	sSystemconfig.accelerationSensorSensitivity = cdr_get_item_value_int(node,"accelerationSensorSensitivity");
	sSystemconfig.graphicCorrect = cdr_get_item_value_int(node,"graphicCorrect");
	sSystemconfig.cdrPowerDown = cdr_get_item_value_int(node,"cdrPowerDown");
	sSystemconfig.eDog = cdr_get_item_value_int(node,"eDog");
	sSystemconfig.bluetooth = cdr_get_item_value_int(node,"bluetooth");
	sSystemconfig.fmFrequency = cdr_get_item_value_int(node,"fmFrequency");
	sSystemconfig.telecontroller = cdr_get_item_value_int(node,"telecontroller");

	cdr_get_item_value_str(node,"rtspLive",sSystemconfig.rtspLive);
	cdr_get_item_value_str(node,"rtspRecord",sSystemconfig.rtspRecord);
	
	sSystemconfig.graphicCorrect = cdr_get_item_value_int(node,"graphicCorrect");
	sSystemconfig.cdrPowerDown = cdr_get_item_value_int(node,"cdrPowerDown");

	node = mxmlFindElement(tree, tree, "cdrSystemInfomation",NULL, NULL,MXML_DESCEND);
	cdr_get_item_value_str(node,"cdrSoftwareVersion",sSystemconfig.sCdrSystemInfo.cdrSoftwareVersion);
	cdr_get_item_value_str(node,"cdrHardwareVersion",sSystemconfig.sCdrSystemInfo.cdrHardwareVersion);
	cdr_get_item_value_str(node,"cdrBuildTime",sSystemconfig.sCdrSystemInfo.cdrBuildTime);
	cdr_get_item_value_str(node,"cdrDeviceSN",sSystemconfig.sCdrSystemInfo.cdrDeviceSN);


	node = mxmlFindElement(tree, tree, "cdrSystemSDFilePath",NULL, NULL,MXML_DESCEND);
	cdr_get_item_value_str(node,"pathPhoto",sSystemconfig.sCdrFilePath.pathPhoto);
	cdr_get_item_value_str(node,"pathIndexPic",sSystemconfig.sCdrFilePath.pathIndexPic);
	cdr_get_item_value_str(node,"pathMp4",sSystemconfig.sCdrFilePath.pathMp4);
	cdr_get_item_value_str(node,"pathXml",sSystemconfig.sCdrFilePath.pathXml);
	cdr_get_item_value_str(node,"pathTrail",sSystemconfig.sCdrFilePath.pathTrail);
	cdr_get_item_value_str(node,"pathSystemLog",sSystemconfig.sCdrFilePath.pathSystemLog);

	
	node = mxmlFindElement(tree, tree, "videoRecord",NULL, NULL,MXML_DESCEND);

	sSystemconfig.sCdrVideoRecord.type = cdr_get_item_value_int(node,"type");
	sSystemconfig.sCdrVideoRecord.mode = cdr_get_item_value_int(node,"mode");
	sSystemconfig.sCdrVideoRecord.rotate = cdr_get_item_value_int(node,"rotate");


	node = mxmlFindElement(tree, tree, "osd",NULL, NULL,MXML_DESCEND);
	sSystemconfig.sCdrOsdCfg.logo = cdr_get_item_value_int(node,"logo");
	sSystemconfig.sCdrOsdCfg.time = cdr_get_item_value_int(node,"time");
	sSystemconfig.sCdrOsdCfg.speed = cdr_get_item_value_int(node,"speed");
	sSystemconfig.sCdrOsdCfg.engineSpeed = cdr_get_item_value_int(node,"engineSpeed");
	sSystemconfig.sCdrOsdCfg.position = cdr_get_item_value_int(node,"position");
	sSystemconfig.sCdrOsdCfg.personalizedSignature = cdr_get_item_value_int(node,"personalizedSignature");
	cdr_get_item_value_str(node,"psString",sSystemconfig.sCdrOsdCfg.psString);


	node = mxmlFindElement(tree, tree, "net",NULL, NULL,MXML_DESCEND);

	sSystemconfig.sCdrNetCfg.wifiMode = cdr_get_item_value_int(node,"wifiMode");
	cdr_get_item_value_str(node,"apSsid",sSystemconfig.sCdrNetCfg.apSsid);
	cdr_get_item_value_str(node,"apPasswd",sSystemconfig.sCdrNetCfg.apPasswd);
	cdr_get_item_value_str(node,"staSsid",sSystemconfig.sCdrNetCfg.staSsid);
	cdr_get_item_value_str(node,"staPasswd",sSystemconfig.sCdrNetCfg.staPasswd);
	sSystemconfig.sCdrNetCfg.dhcp = cdr_get_item_value_int(node,"dhcp");
	cdr_get_item_value_str(node,"ipAddr",sSystemconfig.sCdrNetCfg.ipAddr);
	cdr_get_item_value_str(node,"gateway",sSystemconfig.sCdrNetCfg.gateway);
	cdr_get_item_value_str(node,"netmask",sSystemconfig.sCdrNetCfg.netmask);
	cdr_get_item_value_str(node,"dns",sSystemconfig.sCdrNetCfg.dns);

	memcpy(&g_cdr_systemconfig,&sSystemconfig,sizeof(CDR_SYSTEM_CONFIG));
	mxmlRelease(tree);
	return 0;
	
}

int cdr_config_init(void)
{
	//Read all data to mem
	cdr_read_xml_to_cfg();

	// update wifi info.
	//cdr_init_wifi_config();
	//
	return 0;
}

