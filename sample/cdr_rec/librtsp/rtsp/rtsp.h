#ifndef _RTSP_RTSP_H
#define _RTSP_RTSP_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <linux/errno.h>
#include <errno.h>
#include <semaphore.h>
#include "../comm/type.h"
#include "../comm/version.h"
#include <signal.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include "../utils/OSAL_Queue.h"


#define gettid() syscall(__NR_gettid)

#define RTSP_EL "\r\n"
#define RTSP_VER "RTSP/1.0"
#define HDR_REQUIRE "Require"
#define HDR_ACCEPT "Accept"
#define PACKAGE "ezrtspd"
#define VERSION "1.0"
#define SDP_EL "\r\n"
#define HDR_TRANSPORT "Transport"
#define HDR_SESSION "Session"
#define TRACK       "track"


#define RTSP_BUFFERSIZE 4096
#define MAX_DESCR_LENGTH 4096
#define DEFAULT_TTL 32
#define HDR_CSEQ "CSeq"
#define MAX_CONN 5
#define NB_BUFFER 8
#define SEND_AUDIO
extern sem_t rtspd_semop_sending_data;
extern sem_t rtspd_semop;
extern sem_t h264status;
extern sem_t rtspd_lock[MAX_CONN];
extern sem_t rtp_rtcp_lock[MAX_CONN];
//extern sem_t rtspd_accept_lock;
extern pthread_cond_t rtspd_cond;
extern pthread_mutex_t rtspd_mutex;

#define THREAD_DEBUG
//#define GET_AVDATA_AUDIO_DEBUG

#define RTSP_DEBUG
#ifdef RTSP_DEBUG
#define DBG(fmt, args...) fprintf(stderr, "[rtsp Debug] " fmt, ## args)
#else
#define DBG(fmt, args...)
#endif

#ifdef GET_AVDATA_AUDIO_DEBUG
#define CACHE_AUDIO_DBG(fmt, args...) fprintf(stderr, "[get_avdata Debug] " fmt, ## args)
#else
#define CACHE_AUDIO_DBG(fmt, args...)
#endif

#ifdef GET_AVDATA_VIDEO_DEBUG
#define CACHE_VIDEO_DBG(fmt, args...) fprintf(stderr, "[get_avdata Debug] " fmt, ## args)
#else
#define CACHE_VIDEO_DBG(fmt, args...)
#endif


#ifdef THREAD_DEBUG
#define THREAD_DBG(fmt, args...) fprintf(stderr, "[thread Debug] " fmt, ## args)
#else
#define THREAD_DBG(fmt, args...)
#endif

#define MAX_MEDIO_SESSION 2

extern struct rtsp_buffer *rtsp[MAX_CONN];


struct rtsp_port{
	S32 rtp_udp_track1_port;
	S32 rtp_udp_track2_port;
	S32 rtcp_udp_track1_port;
	S32 rtcp_udp_track2_port;
	S32 rtp_ser_track1_port;
	S32 rtp_ser_track2_port;
	S32 rtcp_ser_track1_port;
	S32 rtcp_ser_track2_port;
    S32 rtp_channel_track1_id;
    S32 rtp_channel_track2_id;
    S32 rtcp_channel_track1_id;
    S32 rtcp_channel_track2_id;
	U32 ssrc;
	U32 timestamp;
	U32 audio_timestamp;
	U32 frame_rate_step;
	U32 audio_step;
	U16 seq;
	U16 audio_seq;
    U32 audio_ssrc;
	U32 video_packet_count;
	U32 video_octet_count;
	U32 audio_packet_count;
	U32 audio_octet_count;
};

typedef struct {
	unsigned char magic;
	unsigned char channel;
	unsigned short leng;
}rtp_frame_t;

struct rtsp_fd{
	S32 rtspfd;
	S32 video_rtp_fd;
	S32 video_rtcp_fd;
	S32 audio_rtp_fd;
	S32 audio_rtcp_fd;

 };


struct rtsp_th{
	pthread_t rtsp_proc_thread;	
	pthread_t rtsp_accept_thread;
	pthread_t rtp_vthread;
	pthread_t rtcp_vthread;
	pthread_t rtcp_vthread1;
    pthread_t rtp_rtcp_init_thread;
};


struct rtsp_cli{
	S32 cli_fd;
	S32 conn_num;
	CHAR cli_host[128];

};

struct rtsp_buffer {

	S32 payload_type; /* 96 h263/h264*/
	S32 session_id;
	U32 rtsp_deport;
	U32 rtsp_um_stat;  /**** 0 is Unicast   1 is multicast  ****/
	U32 rtsp_cseq;
	U32 is_runing;
	U32 client_count;
	U32 conn_status;	
	U32 rtspd_status;
	U32 vist_type;  /****0: H264 file vist  1: PS file vist  2: h264 stream vist ****/
    U32 is_sending_data;
    U32 rtp_tcp_mode;
	U32 link_down;
	U32 timeout_count;
	U32 V_Serial[2];
	U32 A_Serial[2];
	//U32 subType;
	
	U32 rtsp_video_stream; //Milestone request for audio stream
	U32 rtsp_audio_stream;
	
	struct rtsp_port cmd_port;
	struct rtsp_fd fd;
	struct rtsp_th pth;
	struct rtsp_cli cli_rtsp;
	
	// Buffers		
	CHAR file_name[128];
	CHAR host_name[128];
	U8   nalu_buffer[1500];
	CHAR in_buffer[RTSP_BUFFERSIZE];
	CHAR out_buffer[RTSP_BUFFERSIZE];	
	CHAR sdp_buffer[MAX_DESCR_LENGTH];	
};


typedef int (*_rtsp_mediasession_cb)(int nStatus);
struct rtsp_media_session
{
	CHAR media_session_name[128];
	S32 conn_index;
	S32 nend_iframe;//should send i frame.
	void *bufPtr[NB_BUFFER];
	QBuff mBuf[NB_BUFFER];
	OSAL_QUEUE mQueueBuffer;
	pthread_t rtsp_send_thread;
	_rtsp_mediasession_cb mediasession_fn;	
};

extern struct rtsp_media_session g_media_session[MAX_MEDIO_SESSION];

S32 build_rtp_nalu(U8 *inbuffer, S32 frame_size, S32 cur_conn_num,int audio_flag,int audio_type,U32 audio_time,U32 video_time);
S32 build_rtp_tcp_nalu(U8 *inbuffer, S32 frame_size, S32 cur_conn_num,int audio_flag,int audio_type,U32 audio_time,U32 video_time);

//S32 get_sps_pps(CHAR *sps_data,S32 *sps_data_len,CHAR *pps_data,S32 *pps_data_len,int);

#endif
