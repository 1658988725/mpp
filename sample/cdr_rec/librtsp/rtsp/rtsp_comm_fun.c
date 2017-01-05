/*******
 * 
 * FILE INFO: rtsp comm finctions
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/02/9 20:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2011/2/9 22:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
 

#include "rtsp.h"
#include "../comm/type.h"

struct rtsp_buffer *rtsp[MAX_CONN];
struct rtsp_media_session g_media_session[MAX_MEDIO_SESSION];

 /******************************************************************************/
/*
 *	get rtsp  cseg functions 
 * Arguments:
 *     cur_conn_num :    current connect number
 */
/******************************************************************************/
S32 get_rtsp_cseg(S32 cur_conn_num)
{
	CHAR *p;
	CHAR trash[255];
	
	/****check  CSeq****/ 
	if ((p = strstr(rtsp[cur_conn_num]->in_buffer, "CSeq")) == NULL) {
		/**** not find CSeq send 400 error ****/
		send_reply(400,cur_conn_num);
		return -1;
	} 
	else {
		sscanf(p, "%254s %d", trash, &(rtsp[cur_conn_num]->rtsp_cseq));
		//printf("================CSeq:%d\n",rtsp[cur_conn_num]->rtsp_cseq);
		/*
		if(sscanf(p, "%254s %d", trash, &(rtsp[cur_conn_num]->rtsp_cseq))!=2){
			/**** not find CSeq value send 400 error ****/	
	   //		send_reply(400,cur_conn_num);
	   //		return -1;
	   //}
	}
	#if 0
	if( strstr(rtsp[cur_conn_num]->in_buffer, "ClientChallenge")){
		return -1;
	}
	#endif
	return 0;

}

 /******************************************************************************/
/*
 *	check  rtsp  url functions 
 * Arguments:
 *	   cur_conn_num :	 current connect number
 */
/******************************************************************************/
S32 check_rtsp_url(S32 cur_conn_num)
{
	 U16 port;
	 CHAR url[128];
	 CHAR object[128], server[128];
	 

	if (!sscanf(rtsp[cur_conn_num]->in_buffer, " %*s %254s ", url)) {
		send_reply(400,cur_conn_num);	/* bad request */
		return -1;
	}
	/* Validate the URL */
	if (!parse_url(url, server, &port, object)) {
		send_reply(400,cur_conn_num);	/* bad request */
		return -1;
	}

	//printf("check_rtsp_url ----file_name : %s \r\n",object);

	memset(rtsp[cur_conn_num]->file_name,0x00,128);
	strcpy(rtsp[cur_conn_num]->file_name,object);
	
	//printf("=====    port:%d   ====== \n",port);
	strcpy(rtsp[0]->host_name,server);
	
    /*rtsp[0]->subType = 0;
	sscanf(object, "subtype=%d", &(rtsp[0]->subType));	
	if(rtsp[0]->subType != 0)
		rtsp[0]->subType = 1;
	fprintf(stderr, "###################### file_name : %s, subType = %d\n", object, rtsp[cur_conn_num]->subType);*/
#if 0
	/****  get media file name   ****/
	if(strstr(object,"trackID")){
		strcpy(object,rtsp[cur_conn_num]->file_name);
	}
	else{
		if(strcmp(object,"")==0){
			strcpy(object,rtsp[cur_conn_num]->file_name);
		}
		strcpy(rtsp[cur_conn_num]->file_name,object);
	}
#endif
	return 0;
}


 /******************************************************************************/
/*
 *	check  rtsp  filename functions 
 * Arguments:
 *	   cur_conn_num :	 current connect number
 */
/******************************************************************************/
S32 check_rtsp_filename(S32 cur_conn_num)
{
	CHAR *p;
	S32 valid_url;
	
	if (strstr(rtsp[0]->file_name, "../")) {
		/* disallow relative paths outside of current directory. */
		send_reply(403,cur_conn_num);	 /* Forbidden */
		return -1;
	}	 
	if (strstr(rtsp[0]->file_name, "./")) {
		/* Disallow ./ */
		send_reply(403,cur_conn_num);	 /* Forbidden */
		return -1;
	}

	//just support media session add by API
	//cdr_rtsp_create_mediasession("live.sdp",684*480);
	
	int i = 0;
	for(i=0;i<MAX_MEDIO_SESSION;i++)
	{
		//printf("rtsp[%d]->file_name:%s,g_media_session[%d].media_session_name:%s\r\n",i,rtsp[i]->file_name,i,g_media_session[i].media_session_name);
		if (g_media_session[i].media_session_name[0] != 0x00 && 
			strstr(rtsp[cur_conn_num]->file_name,g_media_session[i].media_session_name))
		{
			g_media_session[i].conn_index = cur_conn_num;
			break;
		}
	}
	
	if(i >= MAX_MEDIO_SESSION)
	{
		send_reply(415,cur_conn_num);	 /* Unsupported media type */
		return -1;
	}
	#if 0
	p = strrchr(rtsp[0]->file_name, '.');
	valid_url = 0;
	if (p == NULL) {
		send_reply(415,cur_conn_num);	 /* Unsupported media type */
		return -1;
	} else {
		valid_url = is_supported_mediatype(p,cur_conn_num);
	}
	if (!valid_url) {
		send_reply(415,cur_conn_num);	 /* Unsupported media type */
		return -1;
	}
	#endif
	return 0;
}

S32 update_rtsp_status(S32 cur_conn_num,S32 status)
{
	int ret = 0;
	int i = 0;
	for(i=0;i<MAX_MEDIO_SESSION;i++)
	{
		//printf("rtsp[%d]->file_name:%s,g_media_session[%d].media_session_name:%s\r\n",i,rtsp[i]->file_name,i,g_media_session[i].media_session_name);
		if (g_media_session[i].media_session_name[0] != 0x00 && 
			strstr(rtsp[cur_conn_num]->file_name,g_media_session[i].media_session_name))
		{
			//g_media_session[i].conn_index = cur_conn_num;
			if(g_media_session[i].mediasession_fn)
			{
				g_media_session[i].mediasession_fn(status);
				return 0;
			}
		}
	}
	return ret;
}


