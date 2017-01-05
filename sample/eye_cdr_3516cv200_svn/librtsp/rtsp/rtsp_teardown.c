/*******
 * 
 * FILE INFO: terardown METHOD HANDLING
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/02/1 17:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2011/2/1 18:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
 

#include "rtsp.h"

  
 /******************************************************************************/
 /*
  *  set terardown command  response buffer
  * Arguments:
  *	   cur_conn_num :	 current connect number
  */
 /******************************************************************************/
S32 send_terardown_reply(S32 status,S32 cur_conn_num)
{
	CHAR temp[255];
	int exit_flag;
	if(!rtsp[cur_conn_num]->out_buffer)
		return -1;
	
	/* build a reply message */
	sprintf(rtsp[cur_conn_num]->out_buffer, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL"Server: %s/%s"RTSP_EL, RTSP_VER, status, (CHAR *)get_stat(status), rtsp[cur_conn_num]->rtsp_cseq, PACKAGE,
		VERSION);
	add_time_stamp(rtsp[cur_conn_num]->out_buffer, 0);
	strcat(rtsp[cur_conn_num]->out_buffer, "Session: ");
	sprintf(temp, "%d", rtsp[cur_conn_num]->session_id);
	strcat(rtsp[cur_conn_num]->out_buffer, temp);
	strcat(rtsp[cur_conn_num]->out_buffer, RTSP_EL);
	strcat(rtsp[cur_conn_num]->out_buffer, RTSP_EL);
	if(send_tcp_pkg(rtsp[cur_conn_num]->cli_rtsp.cli_fd, rtsp[cur_conn_num]->out_buffer, strlen(rtsp[cur_conn_num]->out_buffer),0,&exit_flag)<0){
		perror("send_terardown_reply error, ");
		return -1;
	}
	return 0;

}


 /******************************************************************************/
/*
 *	set terardown command  response buffer
 * Arguments:
 *	   cur_conn_num :	 current connect number
 */
/******************************************************************************/
S32 rtsp_terardown(S32 cur_conn_num)
{
	CHAR *p = NULL;
	//CHAR  trash[255];
	if(!rtsp[cur_conn_num]->in_buffer)
		return -1;
	/**** Parse the input message ****/

	if(check_rtsp_url(cur_conn_num)<0){
		return -1;
	}

	if(get_rtsp_cseg(cur_conn_num)<0){
		return -1;
	}
	if(check_rtsp_filename(cur_conn_num)<0){
		return -1;
	}

	// If we get a Session hdr, then we have an aggregate control
	if ((p = strstr(rtsp[cur_conn_num]->in_buffer, HDR_SESSION)) != NULL) {
		//printf("================session_id:%d\n",rtsp[cur_conn_num]->session_id);
		if(send_terardown_reply(200,cur_conn_num)!=-1)
			return 1;
	}
	return -1;
}


