#ifndef _MP4INFO_H
#define _MP4INFO_H

// Tomer Elmalem
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "libutility.h"
#include <mp4v2/mp4v2.h>
#include "cdr_comm.h"
#define MP4DIRPATH "/mnt/mmc/VIDEO"
#define MP4CUTOUTPATH "/mnt/mmc/tmp"
#define MP4GCUTOUTPATH "/mnt/mmc/GVIDEO"

//#pragma pack(1)

typedef int (*stream_out_cb)(const void*);

typedef struct _mp4Info 
{
	char oFileName[256];	// �ļ�����		
	time_t	tmStart;//begin time of mp4.
	time_t  tmEnd;  //endtime of mp4.		
	unsigned long  oStreamDuration;//��Ƶ�ļ�����.-ms
	unsigned char pps[64];
	unsigned char sps[64];
	int ppslen;
	int spslen;
	int nframerate;	
	int videoindex;
	int audioindex;	
	unsigned short ovFrameCount;	
	unsigned short oaFrameCount;	
}Mp4Info;

typedef struct _mp4ReadInfo
{
	char 	chName[256];
	time_t	tmStart;
	time_t	tmEnd;
	int nTotalLen;
	int VReadIndex;
	int AReadIndex;
	stream_out_cb pStreamcb;
	MP4FileHandle oMp4File;
	Mp4Info *oMp4Info;
}Mp4ReadInfo;

typedef struct streamout
{
	int streamType;
	int type_ex;
	unsigned char *pData;
	unsigned int size;	
}StreamOut;



#define VIDEO_TYPE_SPS 	7
#define VIDEO_TYPE_PPS 	8
#define VIDEO_TYPE_IF 	5
#define VIDEO_TYPE_PF 	1

#define STREAM_TYPE_V 	0
#define STREAM_TYPE_A 	1
#define STREAM_TYPE_END 2
#define STREAM_TYPE_START 3


int demuxer_mp4_info(const char *in_filename,Mp4Info *pMp4InfoOut);
int cdr_parsemp4dir(char *path);
int cdr_read_mp4_ex(char *pTm,int len,char* dir,stream_out_cb pstreamout);
int cdr_mp4dirlist_free(void);

int cdr_init_mp4dir(char *path);



#define MP4STREAM_VIDEO 0
#define MP4STREAM_AUDIO 1

typedef struct mp4_frame
{
	int streamType;
	int nFrameType;
	//int nIFrame;
	unsigned long long uPTS;
	int nlen;	
	unsigned char* pData;
}MP4_FRAME;


#define CDR_H264_NALU_PSLICE 1
#define CDR_H264_NALU_ISLICE 5
#define CDR_H264_NALU_SEI 6
#define CDR_H264_NALU_SPS 7
#define CDR_H264_NALU_PPS 8
#define CDR_H264_NALU_IPSLICE 9


typedef int (*newmp4_out_cb)(const void*);

typedef struct _Mp4v2_AConfig
{
	unsigned long nSampleRate;     	//��Ƶ������
	unsigned int nChannels;         //��Ƶ������
	unsigned int nPCMBitSize;       //��Ƶ��������
	unsigned long nInputSamples;    //ÿ�ε��ñ���ʱ��Ӧ���յ�ԭʼ���ݳ���
	unsigned long nMaxOutputBytes;  //ÿ�ε��ñ���ʱ���ɵ�AAC���ݵ���󳤶�
	unsigned char* pbAACBuffer;     //aac����
}MP4V2_ACONFIG;

typedef struct _Mp4v2_VConfig
{
	unsigned int timeScale;        //��Ƶÿ���ticks��,��90000
	unsigned int fps;              //��Ƶ֡��
	unsigned short width;          //��Ƶ��
	unsigned short height;         //��Ƶ��
}MP4V2_VCONFIG;

typedef struct lframenode
{
  struct lframenode* next; /* Pointer to next node */
  void* data; /* User data */
} framenode;


//sPreName �ǹ�����Ƶ��ʱ�����ɵ�ͼƬ����
//��ʽΪ:/mnt/mmc/photo/20161205152900
//�ص�������ļ����ڣ����޸��ļ�����Ϊ/mnt/mmc/photo/20161205152900_123.jpg
// 123 = nCutLen
typedef struct mp4extrainfo
{
	char sPreName[256];	//��ȡ��ƵԤ��ͼƬ����
	int sPrePicFlag;	//sPreName �Ƿ��Ѿ����趨��.
	int nCutFlag;		//��ȡ��Ƶ�ı�־λ
	int nCutFrame;		//Set cut �������ܹ��ж���֡
	int nCutLen;		//��ȡ��Ƶ�ĳ��ȣ���λΪ��.
	int nOutputMp4flag;		//finish ���Ƿ���Ҫ���mp4�ļ�.
}Mp4ExtraInfo;
typedef struct mp4Context
{
	int nIndex;
	char sName[256];		
	int  nlen;//second of video
	int closeFlag;
	int  nNeedAudio;
	unsigned char sps[64];//sps with out nal
	unsigned char pps[64];
	int spslen;
	int ppslen;
	int nFrameleft;//need write nFrameleft to close mp4.	
	framenode *pFrameHead;
	framenode *pFrameTail;
	MP4FileHandle hFile;          //mp4�ļ�������
	MP4TrackId video;              //��Ƶ�����־��
	MP4TrackId audio;              //��Ƶ�����־��	
	MP4V2_ACONFIG oAudioCfg;
	MP4V2_VCONFIG oVideoCfg;	
	newmp4_out_cb outcb;
	Mp4ExtraInfo oMp4ExtraInfo;//��ȡ��Ƶ��صĲ���
	pthread_t tfid;
}Mp4Context;

//SD��¼����صĽӿ�.
//��Ҫ���߳��첽+�ص��ķ�������ֹ¼����Ƶ��ʱ��֡.
//������Ҫ���ƺý��࣬�������¼�Ƶ���Ƶ���Ǵ�I֡��ʼ,����mp4����.
//���Ƿ���Ҫ�������һ��IFrame����.
int cdr_mp4_create(Mp4Context *pMp4Context);
int cdr_mp4_write_frame(int handle,MP4_FRAME *pData);
int cdr_mp4_close(int handle);
int cdr_mp4_checklen(int handle);
int cdr_mp4_setlen(int handle,int len);
int cdr_mp4_setextrainfo(int handle,char* sCutfileName);
int cdr_mp4_getextrainfo(int handle);
int cdr_mp4_outputmp4flag(int handle,int flag);



//��ȡ��Ƶ��Ҫ�Ľӿ�.Ϊ��ֹ��ȡ̫Ƶ�������н�ȡ��ص���Ƶ�������ڲ�һ���߳��ڲ����
int cdr_mp4_create_ex(Mp4Context *pMp4Context);
int cdr_mp4_write_frame_ex(int handle,MP4_FRAME *pData);
int cdr_mp4_close_ex(int handle);


//��REC ����ȡ���ݵĽӿ�
//�ڲ�����ά��һ��ָ�룬��ѭ����ȡ.��������Next frame.
int cdr_mp4ex_read_vframe(char **pFrameData,int *nLen,int *IFrame);



typedef struct _mp4_dir_info
{
	list *fileList; //file list.
	list *cutList;  //file cut list.
	int nThreadStart;
	char chDir[256];
	pthread_t tCutfid;
	pthread_t tDemuxerfid;	
}mp4_dir_info;


#endif
