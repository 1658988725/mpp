#ifndef __CDR_APP_SERVICE_H_
#define __CDR_APP_SERVICE_H_

#include "cdr_protocol.h"
#include "cdr_mpp.h"


int Get_BootVoiceCtrl();
//����ص�����ע��.
//Param:cIndex = �ڼ����ͻ���.
//��������Ļص�����.
int cdr_appinterface_answer(int cmdtype,unsigned short extcmdlen,char *pEtxcmd);

void cdr_app_service_init(void);
void ack_capture_update(char *jpgname);
int cdr_system(const char * cmd);

int cdr_system_reset();
int Set_MuteVolume(unsigned char ucVolMuteFlag);//ao
int Set_AI_MuteVolume(unsigned char ucVolMuteFlag);


int cdr_mp4cut_finish(void* info);



#endif

