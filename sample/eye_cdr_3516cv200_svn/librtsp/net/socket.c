/*******
 * 
 * FILE INFO: creat tcp socket 
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/01/28 15:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2011/01/28 16:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
#include "../rtsp/rtsp.h"
#include "../comm/type.h"
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
 
  /******************************************************************************/
 /*
  *  init rtsp struct memory
  * Arguments:
  */
 /******************************************************************************/

 S32 init_memory()
{
	S32 i;

	for(i=0;i<MAX_CONN;i++){
		rtsp[i]=calloc(1,sizeof(struct rtsp_buffer));
  		if(!rtsp[i]){
			return -1;	
  		}
		sem_init(&rtspd_lock[i], 0, 0);
        sem_init(&rtp_rtcp_lock[i], 0, 1);
	}
	return 0;	
}

/******************************************************************************/
/*
*  free rtsp struct memory
* Arguments:
*/
/******************************************************************************/

S32 free_memory()
{
	S32 i;
	
	for(i=0;i<MAX_CONN;i++){
 		if(rtsp[i]!=NULL){
 			free(rtsp[i]);
			rtsp[i]=NULL;
 		}
		
		sem_destroy(&rtspd_lock[i]);
        sem_destroy(&rtp_rtcp_lock[i]);
	}
	return 0;	 
}


/******************************************************************************/
/*
 * Receiving thread, for rtsp client  command
 * Arguments: NULL
 */
/******************************************************************************/
 void *vd_rtsp_procin(void *arg)
{
	pthread_detach(pthread_self());
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
	while(1)
	{
		  printf("server waiting \n");
		  /*  Accept a connection.	*/
		  tcp_accept();
	}
	pthread_exit(NULL);
}


/******************************************************************************/
/*
 *	create rtsp socket 
 * Arguments:
 *      host    - name of host to which connection is desired
 *      port - port associated with the desired port
 *      type - SOCK_STREAM or SOCK_DGRAM
 */
/******************************************************************************/
S32 create_sercmd_socket(const CHAR *port,S32 type)
{
    /*  Create a socket for the server.  */
    S32 server_len;
    struct sockaddr_in server_address;

	rtsp[0]->fd.rtspfd = socket(AF_INET, type, 0);
	if(rtsp[0]->fd.rtspfd<0){
    	printf("socket() error in create_sercmd_socket.\n" );
		return -1;
	}
    /**** set address reuse ****/
	S32 reuse=1;
  	setsockopt(rtsp[0]->fd.rtspfd, SOL_SOCKET, SO_REUSEADDR, &reuse,sizeof(reuse));
	//tcp
    int send_buf = 32*1024;
	setsockopt(rtsp[0]->fd.rtspfd, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
    int recv_buf = 32*1024;
	setsockopt(rtsp[0]->fd.rtspfd, SOL_SOCKET, SO_RCVBUF, &recv_buf, sizeof(recv_buf));
	//struct timeval timeo = {3, 0};
	//setsockopt(rtsp[0]->fd.rtspfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
	//int flags = fcntl(rtsp[0]->fd.rtspfd, F_GETFL, 0);
	//fcntl(rtsp[0]->fd.rtspfd, F_SETFL, flags | O_NONBLOCK);

  	server_address.sin_family = AF_INET;
  	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  	server_address.sin_port = htons(atoi(port));
  	server_len = sizeof(server_address); 
  	if(bind(rtsp[0]->fd.rtspfd, (struct sockaddr *)&server_address, server_len) < 0)
  	{
		 perror("bind rtsp server port error:\n"); 
		 return -1;
	}
		
  	if(listen(rtsp[0]->fd.rtspfd, MAX_CONN) < 0)
	{
		 perror("listen rtsp server port error:\n"); 
		 return -1;
	}
  	printf("rtsp server listen sucess\n");

  	/***** rtsp cmd thread******/
  	if (pthread_create(&rtsp[0]->pth.rtsp_accept_thread, NULL, vd_rtsp_procin,NULL) < 0){			  
		printf("pthread_create vd_rtsp_procin thread error\n");
	  	return -1;
  	}
	return 0;
}
void rtsp_tcp_para_clear(int cur_conn_num)
{  
    rtsp[cur_conn_num]->cmd_port.rtcp_channel_track1_id = -1;
    rtsp[cur_conn_num]->cmd_port.rtcp_channel_track2_id = -1;
    rtsp[cur_conn_num]->cmd_port.rtp_channel_track1_id = -1;
    rtsp[cur_conn_num]->cmd_port.rtp_channel_track2_id = -1;
    
    if(rtsp[cur_conn_num]->rtp_tcp_mode == RTP_TCP)
    {
        rtsp[cur_conn_num]->rtp_tcp_mode = RTP_UDP;  
    }

	rtp_free(cur_conn_num);
    rtcp_free(cur_conn_num);
	rtsp_clifree(cur_conn_num);
	rtsp[cur_conn_num]->is_sending_data = 0;
}
   

/******************************************************************************/
/*
 * RTSP command match
 *     method:    rtsp command
 *     cur_conn_num:  current  connection status
 */
/******************************************************************************/
S32 rtsp_cmd_match(S32 method, S32 cur_conn_num) 
{
	
	DBG("chn: %d, ip: %s, rtsp_cmd_match, method = %d\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host, method);
	switch(method){
		case 1:
			if(rtsp_options(cur_conn_num) <= 0){
				printf("chn: %d, ip: %s, options command response not sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
				rtsp[cur_conn_num]->rtspd_status=0x01;
                DBG("options error, sem_post\n");
                rtsp_tcp_para_clear(cur_conn_num);
				sem_post(&rtspd_semop_sending_data);
		        sem_post(&rtspd_lock[cur_conn_num]);                
				return -1;
			}
			else{
				printf("chn: %d, ip: %s, options command response  sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				rtsp[cur_conn_num]->rtspd_status=0x02;
			}
			break;
	 
		case 2:
			if(rtsp_describe(cur_conn_num) <= 0){
				printf("chn: %d, ip: %s, describe command response not sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
				rtsp[cur_conn_num]->rtspd_status=0x03;
                rtsp_tcp_para_clear(cur_conn_num);
				sem_post(&rtspd_semop_sending_data);
				sem_post(&rtspd_lock[cur_conn_num]);
				return -1;
			}
			else
			{
				printf("chn: %d, ip: %s, describe command response sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				rtsp[cur_conn_num]->rtspd_status=0x04;
			}
			break;
	
		case 3:
			if(rtsp_setup(cur_conn_num) <= 0){
				printf("chn: %d, ip: %s, setup command response not sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
				rtsp[cur_conn_num]->rtspd_status=0x05;
                rtsp_tcp_para_clear(cur_conn_num);
				sem_post(&rtspd_semop_sending_data);
				sem_post(&rtspd_lock[cur_conn_num]);
	            return -1;
			}
			else{
				printf("chn: %d, ip: %s, setup command response sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				rtsp[cur_conn_num]->rtspd_status=0x06;
			}
			break;
			
		case 4:
			if(rtsp_play(cur_conn_num) <= 0){
				printf("chn: %d, ip: %s, play command response not sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
				rtsp[cur_conn_num]->rtspd_status=0x07;
                rtsp_tcp_para_clear(cur_conn_num);
				sem_post(&rtspd_semop_sending_data);
				sem_post(&rtspd_lock[cur_conn_num]);
		        return -1;
			}
			else{
				printf("chn: %d, ip: %s, play command response sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				/****	deal with  RTP RTCP agreement function ****/
				rtsp[cur_conn_num]->rtspd_status=0x08;
				sem_post(&rtspd_lock[cur_conn_num]);
			}
			break;
		case 5:
			if(rtsp_terardown(cur_conn_num) <= 0){
				printf("chn: %d, ip: %s, terardown command response not sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
				rtsp[cur_conn_num]->rtspd_status=0x09;
                rtsp_tcp_para_clear(cur_conn_num);
    			sem_post(&rtspd_semop_sending_data);
				return -1;
			}
			else{
				printf("chn: %d, ip: %s, terardown command response	sucessful\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
				sem_wait(&rtspd_semop_sending_data);
				rtsp[cur_conn_num]->rtspd_status=0x10;
                rtsp_tcp_para_clear(cur_conn_num);
    			sem_post(&rtspd_semop_sending_data);
				return -1;
			}
			break;
		default:
            if(rtsp[cur_conn_num]->is_sending_data == 1)
            {
                break;
            }
			sem_wait(&rtspd_semop_sending_data);
			rtsp[cur_conn_num]->rtspd_status=0x11;
            rtsp_tcp_para_clear(cur_conn_num);
			sem_post(&rtspd_semop_sending_data);
			sem_post(&rtspd_lock[cur_conn_num]);
		    printf("chn: %d, ip: %s, not match rtsp command\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
		    return -1;
	}


	return 0;
}

 /******************************************************************************/
 /*
  * Receiving thread, for rtsp client  command
  * Arguments: NULL
  */
 /******************************************************************************/
void *vd_rtsp_proc(void *arg)
{
	S32 method,free_conn=0;
	THREAD_DBG("func : %s, line : %d, thread ID : %lu\n", __func__, __LINE__, gettid());
	
	pthread_detach(pthread_self());

	free_conn=(S32)arg;
    DBG("vd_rtsp_proc begin, start read rtsp message\n");
	while(1)
	{
		printf("vd_rtsp_proc : %d, %d\n", free_conn, rtsp[free_conn]->cli_rtsp.cli_fd);
	 	if(tcp_read(free_conn) < 0)
	 	{
			printf("tcp read %d error, ip : %s\n", free_conn, rtsp[free_conn]->cli_rtsp.cli_host);
			perror("read error");
			sem_wait(&rtspd_semop_sending_data);
			rtsp[free_conn]->rtspd_status=0x12;
			rtsp_tcp_para_clear(free_conn);
    		sem_post(&rtspd_semop_sending_data);
		    sem_post(&rtspd_lock[free_conn]);                                                                                                                                          
			break;
		}
		method=get_rtsp_method(free_conn);

					
		if(rtsp_cmd_match(method,free_conn) < 0)
		{
			if(method == 5)
			{
				//rtsp_terardown will return -1 always
				update_rtsp_status(free_conn,method);
			}
			
			break;
		}
	    else
		{	
		
			if(method > 0)
	    	{
				update_rtsp_status(free_conn,method);
	    	}
			
			if(method == 4)
			{			
			    set_free_conn_status(free_conn,1);
				printf("%s %d\r\n",__FUNCTION__,__LINE__);				
			}
			if(method == 5)
			{
			    set_free_conn_status(free_conn,0);
			}
	    }
	}
QUIT:
	set_free_conn_status(free_conn,0);
	printf("chn: %d, ip: %s, thread vd_rtsp_proc exit\n", free_conn, rtsp[free_conn]->cli_rtsp.cli_host);
	pthread_exit(NULL);
	return NULL;
}


/******************************************************************************/
/*
 *	Set free connection status
 * Arguments:
 *     cur_conn :   current  connection 
 *     cur_status:  current  connection status
 */
/******************************************************************************/
S32 set_free_conn_status(S32 cur_conn, S32 cur_status)
{
	#if 0
	S32 i,j=0;

	for(i=0;i<MAX_CONN;i++){
		if((i==cur_conn)){
			rtsp[i]->conn_status=cur_status;	
		}
	}
	#endif
    sem_wait(&rtspd_semop);
    rtsp[cur_conn]->conn_status=cur_status;	
    sem_post(&rtspd_semop);
	return 0;
}


/******************************************************************************/
/*
 *	get free connection status
 * Arguments:
 *     NULL
 */
/******************************************************************************/
S32 get_free_conn_status()
{
	S32 i,j=-1;
    rtsp[0]->client_count = 1;
	for(i=0;i<MAX_CONN;i++)
	{
		if(j == -1 && rtsp[i]->conn_status == 0)
		{
			j = i;	
		}
		else if(rtsp[i]->conn_status == 1)
		{
			printf("client id %d IP %s\n",i,rtsp[i]->cli_rtsp.cli_host);
			rtsp[0]->client_count++;
		}
	}
	return j;	
}


/******************************************************************************/
/*
 *	create tcp_accept 
 * Arguments:
 *     NULL
 */
/******************************************************************************/
S32 tcp_accept()
{
	S32 client_len,free_conn=0;
    struct sockaddr_in client_address;
    S32 accept_fd = 0;

  	client_len = sizeof(client_address);
	//fprintf(stderr, "before accept: %d, %d, %d\n", free_conn, rtsp[free_conn]->cli_rtsp.cli_fd, rtsp[free_conn]->conn_status);
  	accept_fd = accept(rtsp[0]->fd.rtspfd, (struct sockaddr *)&client_address, &client_len);
	if(accept_fd < 0)
	{
		fprintf(stderr, "==========rtsp server accept error==========\n");
		perror("rtsp server accept error");
		kill(getpid(),SIGINT);
		sleep(3);
		fprintf(stderr, "==========rtsp server accept exit==========\n");
		exit(-1);
	}
	printf("=============================================================\n");
	free_conn=get_free_conn_status();
	if(free_conn==-1)
	{
		printf("waring: maximum number of connections\n");
		printf("total client : %d\n", rtsp[0]->client_count - 1);
		close(accept_fd);
		return -1;
	}

	//tcp chennel defult
    rtsp[free_conn]->cmd_port.rtcp_channel_track1_id = -1;
    rtsp[free_conn]->cmd_port.rtcp_channel_track2_id = -1;
    rtsp[free_conn]->cmd_port.rtp_channel_track1_id = -1;
    rtsp[free_conn]->cmd_port.rtp_channel_track2_id = -1;
	//udp port defult
	rtsp[free_conn]->cmd_port.rtp_udp_track1_port = -1;
	rtsp[free_conn]->cmd_port.rtp_udp_track2_port = -1;
	rtsp[free_conn]->cmd_port.rtcp_udp_track1_port = -1;
	rtsp[free_conn]->cmd_port.rtcp_udp_track2_port = -1;
	//---------
	rtsp[free_conn]->cli_rtsp.conn_num = free_conn;
	rtsp[free_conn]->cli_rtsp.cli_fd = accept_fd;
	//--------
	memset(rtsp[free_conn]->in_buffer, '\0', sizeof(rtsp[free_conn]->in_buffer));
	sem_init(&rtspd_lock[free_conn], 0, 0);
    sem_init(&rtp_rtcp_lock[free_conn], 0, 1);
	strcpy(rtsp[free_conn]->cli_rtsp.cli_host,inet_ntoa(client_address.sin_addr));
	printf("now connect client %d ip: %s, total client %d\n", free_conn, rtsp[free_conn]->cli_rtsp.cli_host, rtsp[0]->client_count);
	pthread_mutex_lock(&rtspd_mutex);
	pthread_cond_signal(&rtspd_cond);
	pthread_mutex_unlock(&rtspd_mutex);

	return 0;
}

S32 tcp_read(S32 free_conn)
{
    S32 bytes_read;

	fd_set fds;
	struct timeval timeout;
    //int timeoutCount = 0;

	while(1)
	{		
        timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		FD_ZERO(&fds);
		if(rtsp[free_conn]->cli_rtsp.cli_fd > 0)
			FD_SET(rtsp[free_conn]->cli_rtsp.cli_fd, &fds);
		else
			break;
		switch(select(rtsp[free_conn]->cli_rtsp.cli_fd + 1, &fds, NULL, NULL, &timeout))
		{
			case 0:
				/*if(rtsp[free_conn]->rtp_tcp_mode == RTP_TCP)
				{
    				if(++timeoutCount > 400)
    				{
                        printf("read rtsp socket timeout...........\n");
                        return -1;
    				}
				}*/
				break;
			case -1:
				perror("read rtsp socket timeout, select return -1........");
                return -1;
			default:
				if(rtsp[free_conn]->cli_rtsp.cli_fd > 0 && FD_ISSET(rtsp[free_conn]->cli_rtsp.cli_fd, &fds))
				{
            	    bytes_read = read(rtsp[free_conn]->cli_rtsp.cli_fd,rtsp[free_conn]->in_buffer,sizeof(rtsp[free_conn]->in_buffer));
            		if(bytes_read < 0)
            		{
						perror("tcp_read");
            		    if(EINTR==errno)
            				break;
            			else
            				return (-1);
            		}
            		else
            		{
            			return bytes_read;
            		}
				}
		}
	}
	return -1;
}

S32 tcp_write(S32 fd, void *buf, S32 length)
{
    S32 bytes_write;
	S32 bytes_left = length;
	CHAR *ptr = buf;

	while(bytes_left>0)
	{
	    bytes_write = write(fd,ptr,bytes_left);
		if(bytes_write<0)
		{
		    if(EINTR==errno)
				continue;
			else
				return (-1);
		}
		bytes_left -= bytes_write;
		ptr += bytes_write;
	}

	return (length - bytes_left);
}

