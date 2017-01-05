/************************************************************************	
** Filename: 	cdr_gp.c
** Description:  
** Author: 	xjl
** Create Date: 
** Version: 	v1.0
	Copyright(C) 2016 e-eye CO.LTD. ShenZhen <www.e-eye.cn>
*************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdr_comm.h"
#include <assert.h>
#include <errno.h>

static unsigned char g_cFileName[200] = {0};
static unsigned char g_cDateTime[120] = {0};
static long g_lRecCnt = 0;

extern char g_pCdrStartDateTimeBuf[120];

static int get_cdr_start_time(unsigned char *pDateTimeBuf)
{    
    memcpy(pDateTimeBuf,g_pCdrStartDateTimeBuf,strlen(g_pCdrStartDateTimeBuf)+1);
    
    return 0;
}

static int gp_write_header(unsigned char *pFileName,unsigned char *pDateTimeBuf)
{
   FILE *pfile = NULL;
   unsigned char pcHeadBuf[260] = {0};

   snprintf(pcHeadBuf,sizeof(pcHeadBuf),"StartTime:%s\n",pDateTimeBuf);
   pfile = fopen(pFileName,"a");   
   if(pfile == NULL)
   {
      printf("[%s %d] open file fails\n",__FUNCTION__,__LINE__);
      return -1;
   }   

   fwrite(pcHeadBuf,strlen(pcHeadBuf),1,pfile);
   fflush(pfile); 
   
   if(pfile!=NULL)
   {
    fclose(pfile);
    pfile = NULL;
   }
   return 0;
}

int gp_get_file_name(unsigned char *pucGpFileName)
{

 unsigned char ucGpFileNameBuf[20] = {0};

  sprintf(ucGpFileNameBuf,"gps_%s.gp",g_cDateTime);

  memcpy(pucGpFileName,ucGpFileNameBuf,strlen(ucGpFileNameBuf)+1);
    
  return 0;
}

/*
* function: put one recond to *.gp file
*/
int cdr_add_gp(unsigned char *pucBuff)
{    
   static FILE *pfile = NULL;
   //char cCmdBuf[100] = {0};

   memset(g_cDateTime,0,sizeof(g_cDateTime));
   get_cdr_start_time(g_cDateTime);
   
   memset(g_cFileName,0,sizeof(g_cFileName));
   snprintf(g_cFileName,sizeof(g_cFileName),"/mnt/mmc/GPSTrail/gps_%s.gp",g_cDateTime);

   if((access(g_cFileName, 0)) == -1)
   {
     //printf("access failed, %d, %s\n", errno, strerror(errno));
     gp_write_header(g_cFileName,g_cDateTime);
   }

   if(pfile == NULL)  pfile = fopen(g_cFileName,"a");
   fwrite(pucBuff,strlen(pucBuff),1,pfile);
   fflush(pfile);

   g_lRecCnt++;

#if 1
   fseek(pfile,0L,SEEK_END);  
   int size=ftell(pfile);    
   if((pfile!=NULL) && (size > 4*1024*1024))
   {
    close(pfile);
    pfile = fopen(g_cFileName,"w");
    close(pfile);
    //sprintf(cCmdBuf,"rm -r %s",g_cFileName);
    //cdr_system(cCmdBuf);
   }
#endif   

#if 0
   if(pfile!=NULL)
   {
    close(pfile);
    pfile = NULL;
   }
#endif   

   return 0;
}

//get track all counts
static long cdr_get_recond_count()
{
  return g_lRecCnt;
}

int cdr_add_end_gp_record(void)
{
  long lRecordCnt  = 0;
  char cRecCnt[12] = {0};
  char cGpFullName[150] = {0};
  char cCmdBuf[400] = {0};

  //get the final file name 
  lRecordCnt = cdr_get_recond_count();
  sprintf(cRecCnt,"%ld",lRecordCnt);
  sprintf(cGpFullName,"/mnt/mmc/GPSTrail/gps_%s_%s.gp",g_cDateTime,cRecCnt);
 
  sprintf(cCmdBuf,"mv %s %s",g_cFileName,cGpFullName);
  cdr_system(cCmdBuf);
  memset(cCmdBuf,0,sizeof(cCmdBuf));

  sprintf(cCmdBuf,"zip -qj /mnt/mmc/GPSTrail/gps_%s_%s.zip %s",g_cDateTime,cRecCnt,cGpFullName);
  cdr_system(cCmdBuf);
  memset(cCmdBuf,0,sizeof(cCmdBuf));

  cdr_system("rm -f /mnt/mmc/GPSTrail/*.gp");
  cdr_system("sync");

  return 0;
}



