/*******
 * 
 * FILE INFO: proc rtp rtcp packet use udp agreement
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/04/22  09:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2011/04/22 16:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
#include "../rtsp/rtsp.h"
#include "../rtcp/rtcp.h"
#include "../comm/type.h"

#include <string.h>

extern int audio_enable;

 /******************************************************************************/
 /*
  *  create video rtp socket 
  * Arguments:
  * 	 host	 - name of host to which connection is desired
  * 	 port - port associated with the desired port
  * 	 type - SOCK_STREAM or SOCK_DGRAM
  *	   cur_conn_num :	 current connect number
  */
 /******************************************************************************/
 S32 create_vrtp_socket(const CHAR *host, S32 video_port,S32 audio_port,S32 type,S32 cur_conn_num)
 {
	 /*  Create a socket for the client.	*/
	 S32 len,reuse =1;
	 struct sockaddr_in rtp_video_address,rtp_audio_address,rtp_video_bind,rtp_audio_bind;
	 S32 result;

	 rtsp[cur_conn_num]->fd.video_rtp_fd = socket(AF_INET, type, 0);
	 if(rtsp[cur_conn_num]->fd.video_rtp_fd < 0)
	 {
        return -2;
	 }
     rtsp[cur_conn_num]->fd.audio_rtp_fd = socket(AF_INET, type, 0);
	 if(rtsp[cur_conn_num]->fd.audio_rtp_fd < 0)
	 {
        return -2;
	 }

	 /*set address reuse*/	  
	 (VOID) setsockopt(rtsp[cur_conn_num]->fd.video_rtp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse)); 
     (VOID) setsockopt(rtsp[cur_conn_num]->fd.audio_rtp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse));  
	 
	 /*bind local video port*/		 
	 rtp_video_bind.sin_family = AF_INET; 		 
	 rtp_video_bind.sin_addr.s_addr = htonl(INADDR_ANY);				 
	 rtp_video_bind.sin_port = htons(rtsp[cur_conn_num]->cmd_port.rtp_ser_track1_port);			 
	 if ((bind(rtsp[cur_conn_num]->fd.video_rtp_fd, (struct sockaddr *) &rtp_video_bind, sizeof(rtp_video_bind))) < 0){				 
		 printf("bind rtp server video port error\n"); 
		 return -1;
	 }
     /*bind local audio port*/		 
	 rtp_audio_bind.sin_family = AF_INET; 		 
	 rtp_audio_bind.sin_addr.s_addr = htonl(INADDR_ANY);				 
	 rtp_audio_bind.sin_port = htons(rtsp[cur_conn_num]->cmd_port.rtp_ser_track2_port);			 
	 if ((bind(rtsp[cur_conn_num]->fd.audio_rtp_fd, (struct sockaddr *) &rtp_audio_bind, sizeof(rtp_audio_bind))) < 0){				 
		 printf("bind rtp server audio port error\n"); 
		 return -1;
	 }
	 
	 /*  Name the socket, as agreed with the server.  */
	 rtp_video_address.sin_family = AF_INET;
	 rtp_video_address.sin_addr.s_addr = inet_addr(host);
	 rtp_video_address.sin_port = htons(video_port);
	 len = sizeof(rtp_video_address);
	 
	 /*  Name the socket, as agreed with the server.  */
	 rtp_audio_address.sin_family = AF_INET;
	 rtp_audio_address.sin_addr.s_addr = inet_addr(host);
	 rtp_audio_address.sin_port = htons(audio_port);
	 len = sizeof(rtp_audio_address);
	 
	 /*  Now connect our socket to the server's socket.  */
	 result = connect(rtsp[cur_conn_num]->fd.video_rtp_fd, (struct sockaddr *)&rtp_video_address, len);
	 if(result < 0) {
	 	 printf("connect rtp video socket error\n");
		 return -1;
	 }
	 printf( "chn: %d, ip: %s, connect rtp video socket sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
	 /*  Now connect our socket to the server's socket.  */
	 result = connect(rtsp[cur_conn_num]->fd.audio_rtp_fd, (struct sockaddr *)&rtp_audio_address, len);
	 if(result < 0) {
	 	 printf("connect rtp audio socket error\n");
		 return -1;
	 }
	 printf( "chn: %d, ip: %s, connect rtp audio socket sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);

	 return 0;
 }


/******************************************************************************/
/*
* proc h264 packet thread
* Arguments: NULL
*/
/******************************************************************************/
VOID *vd_rtp_func(VOID *arg)
{
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
	 S32 conn_cur;
	 struct timeval  tv;
	 
	 pthread_detach(pthread_self());
	 conn_cur=(S32)arg;
	// tv.tv_sec = 5;
	 //tv.tv_usec = 0;
	 //setsockopt(rtsp->fd.video_rtp_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	 rtp_send_packet(conn_cur);
 
	 //pthread_join(rtsp_st->rtsp_fd_st.rtp_vthread, NULL);
	 pthread_exit(NULL);
	 return NULL;
}



 /******************************************************************************/
 /*
  *  create video rtcp socket 
  * Arguments:
  * 	 host	 - name of host to which connection is desired
  * 	 port - port associated with the desired port
  * 	 type - SOCK_STREAM or SOCK_DGRAM
  *	   cur_conn_num :	 current connect number
  */
 /******************************************************************************/
 S32 create_vrtcp_socket(const CHAR *host, S32 video_port,S32 audio_port,S32 type,S32 cur_conn_num)
 {
	   /*  Create a socket for the client.	*/
	 S32 len,reuse =1;
	 struct sockaddr_in video_rtcp_address,audio_rtcp_address,video_rtcp_bind,audio_rtcp_bind;
	 S32 result;
	 
	 rtsp[cur_conn_num]->fd.video_rtcp_fd = socket(AF_INET, type, 0);
	 if(rtsp[cur_conn_num]->fd.video_rtcp_fd < 0)
	 {
        return -2;
	 }
     rtsp[cur_conn_num]->fd.audio_rtcp_fd = socket(AF_INET, type, 0);
	 if(rtsp[cur_conn_num]->fd.audio_rtcp_fd < 0)
	 {
        return -2;
	 }
	 
	 /*set address reuse*/	  
	 (VOID) setsockopt(rtsp[cur_conn_num]->fd.video_rtcp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse)); 
     (VOID) setsockopt(rtsp[cur_conn_num]->fd.audio_rtcp_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse));
	 
	 /*bind local video port*/
	 video_rtcp_bind.sin_family = AF_INET;		 
	 video_rtcp_bind.sin_addr.s_addr = htonl(INADDR_ANY); 			 
	 video_rtcp_bind.sin_port = htons(rtsp[cur_conn_num]->cmd_port.rtcp_ser_track1_port);			 
	 if ((bind(rtsp[cur_conn_num]->fd.video_rtcp_fd, (struct sockaddr *) &video_rtcp_bind, sizeof(video_rtcp_bind))) < 0){			 
		 printf("bind rtcp server video port error\n"); 
		 return -1;
	 }
     /*bind local audio port*/
	 audio_rtcp_bind.sin_family = AF_INET;		 
	 audio_rtcp_bind.sin_addr.s_addr = htonl(INADDR_ANY); 			 
	 audio_rtcp_bind.sin_port = htons(rtsp[cur_conn_num]->cmd_port.rtcp_ser_track2_port);			 
	 if ((bind(rtsp[cur_conn_num]->fd.audio_rtcp_fd, (struct sockaddr *) &audio_rtcp_bind, sizeof(audio_rtcp_bind))) < 0){			 
		 printf("bind rtcp server audio port error\n"); 
		 return -1;
	 }
 
	 /*  Name the socket, as agreed with the server.  */
	 video_rtcp_address.sin_family = AF_INET;
	 video_rtcp_address.sin_addr.s_addr = inet_addr(host);
	 video_rtcp_address.sin_port = htons(video_port);
	 len = sizeof(video_rtcp_address);

	 /*  Name the socket, as agreed with the server.  */
	 audio_rtcp_address.sin_family = AF_INET;
	 audio_rtcp_address.sin_addr.s_addr = inet_addr(host);
	 audio_rtcp_address.sin_port = htons(audio_port);
	 len = sizeof(audio_rtcp_address);
	 
	 /*  Now connect our socket to the server's socket.  */
	 result = connect(rtsp[cur_conn_num]->fd.video_rtcp_fd, (struct sockaddr *)&video_rtcp_address, len);
	 if(result < 0) {
		printf("connect rtcp video socket error\n");
		return -1;
	 }

	 printf("chn: %d, ip: %s, connect rtcp video socket sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);

	 /*  Now connect our socket to the server's socket.  */
	 result = connect(rtsp[cur_conn_num]->fd.audio_rtcp_fd, (struct sockaddr *)&audio_rtcp_address, len);
	 if(result < 0) {
		printf("connect rtcp audio socket error\n");
		return -1;
	 }
	 printf("chn: %d, ip: %s, connect rtcp audio socket sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
	 return 0;
 }
 void *vd_rtcp_tcp_send(void *arg)
{
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
    S32 len, video_len = 0,audio_len = 0,cur_conn;
    cur_conn=(S32)arg;
    int video_flag = 1; 
	CHAR rtcp_packet[256]={0};
    CHAR rtcp_audio_packet[128]={0};
	pthread_detach(pthread_self());
	int socket = -1;
	fd_set fds;
	struct timeval timeout;
	cur_conn=(S32)arg;
    //int count =0 ;
    sleep(1);
	while(rtsp[cur_conn]->is_sending_data)
    {    
        usleep(5000000);
        //sem_wait(&rtp_rtcp_lock[cur_conn]);
        if(rtsp[cur_conn]->cmd_port.rtcp_channel_track1_id != -1){
            video_flag = 1;
            video_len = rtcp_tcp_send_packet(rtcp_packet,cur_conn,video_flag);    
        }else{
            video_len = 0;
        }
		if(audio_enable == 1){

            if(rtsp[cur_conn]->cmd_port.rtcp_channel_track2_id != -1){
                video_flag = 0;        
                audio_len=rtcp_tcp_send_packet(rtcp_audio_packet,cur_conn,video_flag); 
            }else{
                audio_len = 0;
            }

            if(audio_len!=0){
				printf("%s----->%d  \n", __FUNCTION__, __LINE__);
                memcpy(rtcp_packet+video_len,rtcp_audio_packet,audio_len);
            }
		}else{
			audio_len = 0;
		}
        len=audio_len+video_len;

		if(len >0){
            if(send(rtsp[cur_conn]->fd.video_rtcp_fd, rtcp_packet,len,0) != len){

                printf("rtcp chn: %d ip : %s, send rtcp audio packet error\n", cur_conn, rtsp[cur_conn]->cli_rtsp.cli_host);
				printf("close client------->%s \n",rtsp[cur_conn]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
    			rtsp_tcp_para_clear(cur_conn);
    			sem_post(&rtspd_semop_sending_data);
				
            }  
		}
        //sem_post(&rtp_rtcp_lock[cur_conn]);       

    }

	printf("rtcp chn: %d ip : %s, tcp send thread exit\n", cur_conn, rtsp[cur_conn]->cli_rtsp.cli_host);
    //pthread_join(rtsp_st->rtsp_fd_st.rtcp_vthread, NULL);
    update_rtsp_status(cur_conn,5);
    pthread_exit(NULL);
 	
    return NULL;	
}


void *vd_rtcp_send(void *arg)
{
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
    S32 len, cur_conn;
    cur_conn=(S32)arg;
    int video_flag = 1; 
	CHAR rtcp_packet[128]={0};
	pthread_detach(pthread_self());
	int socket = -1;
	fd_set fds;
	struct timeval timeout;
    int Count = 0;
	cur_conn=(S32)arg;
    //int count =0 ;
	while(rtsp[cur_conn]->is_sending_data)
    {

        usleep(100000);

		if(++Count >= 50) //可疑点
	    {
           
			if(rtsp[cur_conn]->cmd_port.rtcp_udp_track1_port != -1){
				video_flag = 1;
                len=rtcp_send_packet(rtcp_packet,cur_conn,video_flag);
                if(write(rtsp[cur_conn]->fd.video_rtcp_fd,rtcp_packet,len)<0){
                    printf("rtcp chn: %d ip : %s, send rtcp video packet error\n", cur_conn, rtsp[cur_conn]->cli_rtsp.cli_host);
    				perror("[error] write:");
					printf("close client------->%s \n",rtsp[cur_conn]->cli_rtsp.cli_host);
				    sem_wait(&rtspd_semop_sending_data);
        			rtsp_tcp_para_clear(cur_conn);
        			sem_post(&rtspd_semop_sending_data);
                    //rtsp[cur_conn]->rtspd_status=0x21;
                }
			}
			if((audio_enable == 1) && (rtsp[cur_conn]->cmd_port.rtcp_udp_track2_port != -1)){
                video_flag = 0;
                len=rtcp_send_packet(rtcp_packet,cur_conn,video_flag);
				
                if(write(rtsp[cur_conn]->fd.audio_rtcp_fd, rtcp_packet,len)<0){
                    printf("rtcp chn: %d ip : %s, send rtcp audio packet error\n", cur_conn, rtsp[cur_conn]->cli_rtsp.cli_host);
					perror("[error] write:");
					printf("close client------->%s \n",rtsp[cur_conn]->cli_rtsp.cli_host);
				    sem_wait(&rtspd_semop_sending_data);
        			rtsp_tcp_para_clear(cur_conn);
        			sem_post(&rtspd_semop_sending_data);
                    //rtsp[cur_conn]->rtspd_status=0x22;
                }
			}

		    Count = 0;
		}
    }
	//rtcp_free(cur_conn);
	printf("rtcp chn: %d ip : %s, udp send thread exit\n", cur_conn, rtsp[cur_conn]->cli_rtsp.cli_host);
    //pthread_join(rtsp_st->rtsp_fd_st.rtcp_vthread, NULL);
    pthread_exit(NULL);
 	
    return NULL;	
}
 /******************************************************************************/
 /*
  * write receiver report and read sender report
  * Arguments: NULL
  */
 /******************************************************************************/
VOID *vd_rtcp_recv(VOID *arg)
{
	 THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
	 S32 cur_conn;	 
	 CHAR rtcp_read[128];
	 int socket = -1;
	 fd_set fds;
	 struct timeval timeout;
	 pthread_detach(pthread_self());
     int timeoutCount = 0;
	 cur_conn=(S32)arg;
	 
	 while(rtsp[cur_conn]->is_sending_data){

        timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		FD_ZERO(&fds);
	 
		if(rtsp[cur_conn]->fd.video_rtcp_fd > 0)
		{
			FD_SET(rtsp[cur_conn]->fd.video_rtcp_fd, &fds);
            socket = rtsp[cur_conn]->fd.video_rtcp_fd;
		}
		if(audio_enable == 1){
    		if(rtsp[cur_conn]->fd.audio_rtcp_fd > 0){
    			FD_SET(rtsp[cur_conn]->fd.audio_rtcp_fd, &fds);
    			socket = rtsp[cur_conn]->fd.audio_rtcp_fd > rtsp[cur_conn]->fd.video_rtcp_fd ? rtsp[cur_conn]->fd.audio_rtcp_fd : rtsp[cur_conn]->fd.video_rtcp_fd;
			}
		}
	
		
		switch(select(socket + 1, &fds, NULL, NULL, &timeout))
		{
			case 0:
			case -1:
#if 0           //为接入Milestone不对接收rtcp数据进行判断
				if(++timeoutCount > 100)
				{
			        rtsp_clifree(cur_conn);
        			rtsp[cur_conn]->is_runing=0;
                    rtp_free(cur_conn);
					sem_wait(&rtspd_semop_sending_data);
        			rtsp[cur_conn]->is_sending_data = 0;
        			sem_post(&rtspd_semop_sending_data);
    				printf("can't recv rtcp message, client %d logout\n", cur_conn);
				}
#endif
				break;
			default:
				
				if(rtsp[cur_conn]->fd.video_rtcp_fd > 0 && FD_ISSET(rtsp[cur_conn]->fd.video_rtcp_fd, &fds))
				{
                    if(read(rtsp[cur_conn]->fd.video_rtcp_fd,rtcp_read,128)<0){
            		 	perror("read rtcp video packet error\n");
            			//rtsp[cur_conn]->rtspd_status=0x23;
            		}
					else
					    timeoutCount = 0;
				}
				if(socket < 0)
		            printf("[ -- test -- ] socket:%d\n",socket);
				if((audio_enable == 1) && (rtsp[cur_conn]->fd.audio_rtcp_fd > 0)&& FD_ISSET(rtsp[cur_conn]->fd.audio_rtcp_fd, &fds))
				{
            		if(read(rtsp[cur_conn]->fd.audio_rtcp_fd,rtcp_read,128)<0){
            		 	perror("read rtcp audio packet error\n");
            			//rtsp[cur_conn]->rtspd_status=0x24;
            		}
					else
					    timeoutCount = 0;
				}
				break;
		}
	 }

	 //rtcp_free(cur_conn);
	 //set_free_conn_status(cur_conn,0);
	 printf("rtcp chn: %d, ip : %s, udp recv thread exit\n", cur_conn, rtsp[cur_conn]->cli_rtsp.cli_host);

	 //pthread_join(rtsp_st->rtsp_fd_st.rtcp_vthread, NULL);
	 pthread_exit(NULL);
	 return NULL;
}


 /******************************************************************************/
/*
 *deal with your RTP data packing and subdivision 
 *	   cur_conn_num :	 current connect number
 */
/******************************************************************************/
S32 proc_rtp(S32 cur_conn_num)
{
	if(create_vrtp_socket(rtsp[cur_conn_num]->cli_rtsp.cli_host,
		rtsp[cur_conn_num]->cmd_port.rtp_udp_track1_port,rtsp[cur_conn_num]->cmd_port.rtp_udp_track2_port,
		SOCK_DGRAM,
		cur_conn_num))
	{
	    printf("Create vrtp socket error!\n");
		//rtsp[cur_conn_num]->rtspd_status=0x13;
		return -1;
	}
	//rtsp[cur_conn_num]->rtspd_status=0x14;
	if (pthread_create(&rtsp[cur_conn_num]->pth.rtp_vthread,NULL,vd_rtp_func,(VOID *)cur_conn_num) < 0){
		printf("pthread_create rtcp error:\n");
		return -1;
	}

	return 0;
}


 /******************************************************************************/
/*
 * deal with rtcp rr and sr sdec bye packet
 *	   cur_conn_num :	 current connect number
 */
/******************************************************************************/
S32 proc_rtcp(S32 cur_conn_num)
{

    /*TCP模式下，不需要再创建线程收RTCP的数据*/
	if(rtsp[cur_conn_num]->rtp_tcp_mode != RTP_TCP)
    {
		
        if(create_vrtcp_socket(rtsp[cur_conn_num]->cli_rtsp.cli_host,
            rtsp[cur_conn_num]->cmd_port.rtcp_udp_track1_port,rtsp[cur_conn_num]->cmd_port.rtcp_udp_track2_port,
            SOCK_DGRAM,
            cur_conn_num))
        {
            printf("Create vrtcp socket error!\n");
            return -1;
        }
        /*
     	if (pthread_create(&rtsp[cur_conn_num]->pth.rtcp_vthread, NULL, vd_rtcp_recv,(VOID *)cur_conn_num) < 0){			
    		printf("pthread_create rtcp error:\n");
    		return -1;
    	}   
    	*/
    }   

    if (pthread_create(&rtsp[cur_conn_num]->pth.rtcp_vthread1, NULL, vd_rtcp_send,(VOID *)cur_conn_num) < 0){			
		printf("pthread_create rtcp error:\n");
		return -1;
	}
	
	return 0;
}

