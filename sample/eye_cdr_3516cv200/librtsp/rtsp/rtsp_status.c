/*******
 * 
 * FILE INFO: get rtsp status tables
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/01/28 18:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2011/01/28 18:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
 //#include <stdio.h>
 #include "rtsp.h"
 #include "../comm/type.h"
 
 
 /******************************************************************************/
/*
 *	get rtsp status tables 
 * Arguments:
 *     err     err number
 */
/******************************************************************************/
CHAR *get_stat(S32 err)
{
	struct {
		CHAR *token;
		S32 code;
	} status[] = {
		{
		"Continue", 100}, {
		"OK", 200}, {
		"Created", 201}, {
		"Accepted", 202}, {
		"Non-Authoritative Information", 203}, {
		"No Content", 204}, {
		"Reset Content", 205}, {
		"Partial Content", 206}, {
		"Multiple Choices", 300}, {
		"Moved Permanently", 301}, {
		"Moved Temporarily", 302}, {
		"Bad Request", 400}, {
		"Unauthorized", 401}, {
		"Payment Required", 402}, {
		"Forbidden", 403}, {
		"Not Found", 404}, {
		"Method Not Allowed", 405}, {
		"Not Acceptable", 406}, {
		"Proxy Authentication Required", 407}, {
		"Request Time-out", 408}, {
		"Conflict", 409}, {
		"Gone", 410}, {
		"Length Required", 411}, {
		"Precondition Failed", 412}, {
		"Request Entity Too Large", 413}, {
		"Request-URI Too Large", 414}, {
		"Unsupported Media Type", 415}, {
		"Bad Extension", 420}, {
		"Invalid Parameter", 450}, {
		"Parameter Not Understood", 451}, {
		"Conference Not Found", 452}, {
		"Not Enough Bandwidth", 453}, {
		"Session Not Found", 454}, {
		"Method Not Valid In This State", 455}, {
		"Header Field Not Valid for Resource", 456}, {
		"Invalid Range", 457}, {
		"Parameter Is Read-Only", 458}, {
		"Unsupported transport", 461}, {
		"Internal Server Error", 500}, {
		"Not Implemented", 501}, {
		"Bad Gateway", 502}, {
		"Service Unavailable", 503}, {
		"Gateway Time-out", 504}, {
		"RTSP Version Not Supported", 505}, {
		"Option not supported", 551}, {
		"Extended Error:", 911}, {
		NULL, -1}
	};
	S32 i;
	for (i = 0; status[i].code != err && status[i].code != -1; ++i);
	return status[i].token;
}


 /******************************************************************************/
/*
 *	send rtsp error response mseeage 
 * Arguments:
 *     errornumber   rtsp command  err number
 *     cur_conn_num :    current connect number
 */
/******************************************************************************/

S32 send_reply(S32 errornumber,S32 cur_conn_num)
{
	 CHAR error_buff[1024]="";
	 int exit_flag;
	 sprintf(error_buff, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL, RTSP_VER, errornumber, (CHAR *)get_stat(errornumber), rtsp[cur_conn_num]->rtsp_cseq);
	 //---patch coerenza con rfc in caso di errore
	 // strcat(b, "\r\n");
	 strcat(error_buff, RTSP_EL);
	 if(send_tcp_pkg(rtsp[cur_conn_num]->cli_rtsp.cli_fd, error_buff, strlen(error_buff),0,&exit_flag)<0){
	 	perror("send rtspd reply packet error");
		return -1;
 	}

	return 0;
}
#define OUTPUT(fmt, args...)	{\
	FILE *efp = fopen("/mnt/nand/rtspdebug.txt", "a");\
	fprintf(efp, fmt, ##args);\
	fclose(efp);\
} 

 /******************************************************************************/
/*
 *	obtain RTSP command type  
 * Arguments:
 *     inbuf     receive rtsp client buff
 *     cur_conn_num :    current connect number
 */
/******************************************************************************/
S32 get_rtsp_method(S32 cur_conn_num)
{
	char method[32];
	S32 cmd = 0;
	
	if(!rtsp[cur_conn_num]->in_buffer)
		return -1;
    memset(method,'\0',sizeof(method));
	sscanf(rtsp[cur_conn_num]->in_buffer, "%31s", method);
	//OUTPUT("%s\n",method);
	if (strcmp(method, "OPTIONS") == 0) {
		printf("RTSP CMD ---> OPTIONS \r\n");
		cmd = 1;
	}else if (strcmp(method, "DESCRIBE") == 0) {
			printf("RTSP CMD ---> DESCRIBE \r\n");
		cmd = 2;
	}else if (strcmp(method, "SETUP") == 0) {
			printf("RTSP CMD ---> SETUP \r\n");
		cmd = 3;
	}else if (strcmp(method, "PLAY") == 0) {
			printf("RTSP CMD ---> PLAY \r\n");
		cmd = 4;
	}else if (strcmp(method, "TEARDOWN") == 0) {
			printf("RTSP CMD ---> TEARDOWN \r\n");
		cmd = 5;
	}
	//OUTPUT("%d\n",cmd);
	return cmd;
 }
