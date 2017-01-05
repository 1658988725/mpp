
/*******
 * 
 * FILE INFO: DESCRIBE METHOD HANDLING
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/01/28 17:14:26
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

#include "rtsp.h"
#include "../comm/type.h"
//#include "AWcodecTest.h"
#include <assert.h>

/* Routines to encode and decode base64 text.
 */
#include <ctype.h>
#include <string.h>
//#include <cmem.h>

extern int audio_encode_type;
extern int audio_enable;


  /******************************************************************************/
 /*
  *  parse	URL  
  * Arguments:
  * 	url  
  * 	server: 	   server ip addres
  *   port: 		server port
  *   file_name:  visit filename
  */
 /******************************************************************************/

 S32 is_supported_mediatype(CHAR *p,S32 cur_conn_num)
 /* Add supported format extensions here*/
 {
	 if (strcasecmp(p,".264")==0) {	 
	 	rtsp[cur_conn_num]->vist_type=0;
		return 1;
	 }	 
	 if (strcasecmp(p,".H264")==0) {	 
		rtsp[cur_conn_num]->vist_type=0;
		return 1;
	 }	
	 if (strstr(p,"h264stream")) {
	 	 rtsp[cur_conn_num]->vist_type=2;
		 return 1;
	 }
	 if (strcasecmp(p,".ps")==0){
		rtsp[cur_conn_num]->vist_type=1;
		return 1;
	 }


	 return 0;
 }


 /******************************************************************************/
/*
 *	parse  URL	
 * Arguments:
 *	   url	
 *	   server:		  server ip addres
 *	 port:		   server port
 *	 file_name:  visit filename
 */
/******************************************************************************/

/* return 1 if the URL is valid, 0 otherwise*/
S32 parse_url(const CHAR *url, CHAR *server, U16 *port, CHAR *file_name)
// Note: this routine comes from OMS
{
	/* expects format '[rtsp://server[:port/]]filename' */

	S32 valid_url = 0;
	CHAR *token,*port_str;
	CHAR temp_url[128]="";
	
	/* copy url */
	strcpy(temp_url, url);
	if (strncmp(temp_url, "rtsp://", 7) == 0) {
	   	token = strtok(&temp_url[7], " :/\t\n");
	   	strcpy(server, token);
	   	port_str = strtok(&temp_url[strlen(server) + 7 + 1], " /\t\n");
	   	if (port_str)
			*port = (U16) atol(port_str);
		else
			*port = 554;
		valid_url = 1;

		token = strtok(NULL, " ");
		if (token)
			strcpy(file_name, token);
		else
			file_name[0] = '\0';

	} else {
		/* try just to extract a file name */
	}
	
	return valid_url;
}

CHAR *get_hostname(){
	S32 l;
	
	gethostname(rtsp[0]->host_name,sizeof(rtsp[0]->host_name));
	l=strlen(rtsp[0]->host_name);
	//if (getdomainname(rtsp[0]->host_name+l+1,sizeof(rtsp[0]->host_name)-l)!=0) {
	//	rtsp[0]->host_name[l]='.';
	//}
	//strcpy(rtsp[0]->host_name, "chendan");
	return rtsp[0]->host_name;
}

/******************************************************************************/
/*
 *	create time_stamp
 * Arguments:
 *     b:     time_stamp buffer
 *     crlf     
 */
/******************************************************************************/
VOID add_time_stamp(CHAR *b, S32 crlf)
	 {
		 struct tm *t;
		 time_t now;
	 
		 /*
		  * concatenates a null terminated string with a
		  * time stamp in the format of "Date: 23 Jan 1997 15:35:06 GMT"
		  */
		 now = time(NULL);
		 t = gmtime(&now);
		 strftime(b + strlen(b), 38, "Date: %a, %d %b %Y %H:%M:%S GMT"RTSP_EL, t);
		 if (crlf)
			 strcat(b, "\r\n");  /* add a message header terminator (CRLF) */
}

 
/******************************************************************************/
/*
 *	set describe command  response buffer
 * Arguments:
 *     status   rtsp status
 *     cur_conn_num :    current connect number
 */
/******************************************************************************/
S32 send_describe_reply(S32 status,S32 cur_conn_num){

	int exit_flag;
	if(!rtsp[cur_conn_num]->out_buffer)
		return -1;
	/*describe*/
	sprintf(rtsp[cur_conn_num]->out_buffer, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL"Server: %s/%s"RTSP_EL, RTSP_VER, status, (CHAR *)get_stat(status), rtsp[cur_conn_num]->rtsp_cseq, PACKAGE, VERSION);	
	add_time_stamp(rtsp[cur_conn_num]->out_buffer, 0);
	strcat(rtsp[cur_conn_num]->out_buffer, "Content-Type: application/sdp"RTSP_EL);
	sprintf(rtsp[cur_conn_num]->out_buffer + strlen(rtsp[cur_conn_num]->out_buffer), "Content-Base: rtsp://%s:%d/%s"RTSP_EL,rtsp[0]->host_name, rtsp[0]->rtsp_deport, rtsp[0]->file_name);
	sprintf(rtsp[cur_conn_num]->out_buffer + strlen(rtsp[cur_conn_num]->out_buffer), "Content-Length: %d"RTSP_EL, strlen(rtsp[cur_conn_num]->sdp_buffer));
	strcat(rtsp[cur_conn_num]->out_buffer, RTSP_EL);
	/**** concatenate description ****/
	strcat(rtsp[cur_conn_num]->out_buffer, rtsp[cur_conn_num]->sdp_buffer);
	
	if(send_tcp_pkg(rtsp[cur_conn_num]->cli_rtsp.cli_fd, rtsp[cur_conn_num]->out_buffer, strlen(rtsp[cur_conn_num]->out_buffer),0,&exit_flag)<0){
		perror("send_describe_reply error:");
		return -1;
	}
	return 0;
}


  /******************************************************************************/
 /*
  *  get  sdp   message 
  * Arguments:
  * 	
  * 	
  */
 /******************************************************************************/
 CHAR *get_SDP_user_name(CHAR *buffer)
 {
	 strcpy(buffer,PACKAGE);
	 return buffer;
 }

 float NTP_time(time_t t)
 {
	 return (float)t+2208988800U;
 }

 CHAR *get_SDP_session_id(CHAR *buffer)
 {	 
	 buffer[0]='\0';
	 sprintf(buffer,"%.0f",NTP_time(time(NULL)));
	 return buffer;  
 }

 CHAR *get_SDP_version(CHAR *buffer)
 {
	 buffer[0]='\0'; 
	 sprintf(buffer,"%.0f",NTP_time(time(NULL)));
	 return buffer;
 }

 CHAR *get_address()
 {
   static CHAR	   Ip[256];
   CHAR server[256];
   u_char		   addr1, addr2, addr3, addr4;
   u_long		   InAddr;
   struct hostent *host;

   gethostname(server,256);
   host = gethostbyname(server);
 
   InAddr = *(U32 *) host->h_addr;
   addr4 = (U8) ((InAddr & 0xFF000000) >> 0x18);
   addr3 = (U8) ((InAddr & 0x00FF0000) >> 0x10);
   addr2 = (U8) ((InAddr & 0x0000FF00) >> 0x8);
   addr1 = (U8) (InAddr & 0x000000FF);
 
   sprintf(Ip, "%d.%d.%d.%d", addr1, addr2, addr3, addr4);
   return Ip;
 }
 
/* RFC 2045 section 6.8 */

static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			     "abcdefghijklmnopqrstuvwxyz"
			     "0123456789"
			     "+/";

static const char index_64[128] =
  {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  };

/* Decode srclen bytes of base64 data contained in src and put the result
   in dst.  Since the destination buffer may contain arbitrary binary
   data and it is not necessarily a string, there is no \0 byte at the
   end of the decoded data. */
int
b64_decode (void *dst, int dstlen, const char *src, int srclen)
{
  const unsigned char *p, *q;
  unsigned char *t;
  int c1, c2;

  assert (dst != NULL && dstlen > 0);

  if (src == NULL)
    return 0;

  if (srclen < 0)
    srclen = strlen (src);

  /* Remove leading and trailing white space */
  for (p = (const unsigned char *) src;
       srclen > 0 && isspace (*p);
       p++, srclen--)
    ;
  for (q = p + srclen - 1; q >= p && isspace (*q); q--, srclen--)
    ;

  /* Length MUST be a multiple of 4 */
  if (srclen % 4 != 0)
    return -1;

  /* Destination buffer length must be sufficient */
  if (srclen / 4 * 3 + 1 > dstlen)
    return -1;

  t = dst;
  while (srclen > 0)
    {
      srclen -= 4;
      if (*p >= 128 || (c1 = index_64[*p++]) == -1)
        return -1;
      if (*p >= 128 || (c2 = index_64[*p++]) == -1)
        return -1;
      *t++ = (c1 << 2) | ((c2 & 0x30) >> 4);

      if (p[0] == '=' && p[1] == '=')
        break;
      if (*p >= 128 || (c1 = index_64[*p++]) == -1)
	return -1;
      *t++ = ((c2 & 0x0f) << 4) | ((c1 & 0x3c) >> 2);

      if (p[0] == '=')
	break;
      if (*p >= 128 || (c2 = index_64[*p++]) == -1)
        return -1;
      *t++ = ((c1 & 0x03) << 6) | c2;
    }

  return t - (unsigned char *) dst;
}

/* Return a pointer to a base 64 encoded string.  The input data is
   arbitrary binary data, the output is a \0 terminated string.
   src and dst may not share the same buffer.  */
int
b64_encode (char *dst, int dstlen, const void *src, int srclen)
{
  char *to = dst;
  const unsigned char *from;
  unsigned char c1, c2;
  int dst_needed;

  assert (dst != NULL && dstlen > 0 && srclen >= 0);

  if (src == NULL)
    return 0;

  dst_needed = (srclen + 2) / 3;
  dst_needed *= 4;
  if (dstlen < dst_needed + 1)
    return -1;

  from = src;
  while (srclen > 0)
    {
      c1 = *from++; srclen--;
      *to++ = base64[c1 >> 2];
      c1 = (c1 & 0x03) << 4;
      if (srclen <= 0)
	{
	  *to++ = base64[c1];
	  *to++ = '=';
	  *to++ = '=';
	  break;
	}
      c2 = *from++; srclen--;
      c1 |= (c2 >> 4) & 0x0f;
      *to++ = base64[c1];
      c1 = (c2 & 0x0f) << 2;
      if (srclen <= 0)
	{
	  *to++ = base64[c1];
	  *to++ = '=';
	  break;
	}
      c2 = *from++; srclen--;
      c1 |= (c2 >> 6) & 0x03;
      *to++ = base64[c1];
      *to++ = base64[c2 & 0x3f];
    }
  *to = '\0';
  return to - dst;
}
/*
S32 get_sps_pps(CHAR *sps_data,S32 *sps_data_len,CHAR *pps_data,S32 *pps_data_len,int video_type)
{
    int nalu_len;

	return nalu_len;

}*/

 void get_sps_pps(char * data,int *sps_len)
 {
#if 0 
	 printf("----->%d  \n",__LINE__);
	 if(sps_pps_data != NULL) {
		 printf("--------%d",sps_pps_data_length);
		 *sps_len = sps_pps_data_length;
		 memcpy(data, sps_pps_data, sps_pps_data_length);
	 }
#endif	 
 }

 /******************************************************************************/
/*
 *	get  describe sdp message 
 * Arguments:
 *     cur_conn_num :    current connect number
 *     
 */
/******************************************************************************/
#if 1
int global_sps = 0;
static CHAR sps_base[544];

S32 get_describe_sdp(CHAR *sdp_buff,S32 cur_conn_num,int video_type)
{
	CHAR s[30];
	int i,sps_ret,pps_ret;
	CHAR sps_data[544];	
	static S32  sps_len;
	//CHAR pps_data[32];
	static S32  pps_len;
	S32  sps_out_len;
	S32  pps_out_len;
	//printf("----->%d  \n",__LINE__);	
	if(!sdp_buff)
		return -1;
	strcpy(sdp_buff, "v=0"SDP_EL);	
	strcat(sdp_buff, "o=");
	strcat(sdp_buff, "- 946684812024775 1 ");
	strcat(sdp_buff, "IN ");		/* Network type: Internet. */
   	strcat(sdp_buff, "IP4 ");		/* Address type: IP4. */
	//printf("----->%d  \n",__LINE__);
	strcat(sdp_buff, rtsp[0]->host_name);
	//printf("----->%d  \n",__LINE__);
	strcat(sdp_buff, SDP_EL);
	strcat(sdp_buff, "s=RTSP/RTP stream from IPNC"SDP_EL);
	strcat(sdp_buff, "i=2?videoCodecType=H.264"SDP_EL);
   	//sprintf(sdp_buff + strlen(sdp_buff), "u=%s"SDP_EL, rtsp[0]->file_name);
   	strcat(sdp_buff, "t=0 0"SDP_EL);
	strcat(sdp_buff, "a=tool:LIVE555 Streaming Media v2011.05.25"SDP_EL);
	
	strcat(sdp_buff, "a=type:broadcast");
	strcat(sdp_buff, SDP_EL);
	strcat(sdp_buff, "a=control:*");
	strcat(sdp_buff, SDP_EL);
	strcat(sdp_buff, "a=range:npt=0-");
	strcat(sdp_buff, SDP_EL);
	strcat(sdp_buff, "a=x-qt-text-nam:RTSP/RTP stream from IPNC");
	strcat(sdp_buff, SDP_EL);
    strcat(sdp_buff, "a=x-qt-text-inf:2?videoCodecType=H.264");
	strcat(sdp_buff, SDP_EL);
	/**** media specific ****/
	strcat(sdp_buff, "m=");
	strcat(sdp_buff, "video 0");
	strcat(sdp_buff, " RTP/AVP 96"); /* Use UDP */
	strcat(sdp_buff, SDP_EL);
	strcat(sdp_buff, "c=IN IP4 0.0.0.0 ");
	strcat(sdp_buff, SDP_EL);
	strcat(sdp_buff, "b=AS:12000 ");
    strcat(sdp_buff, SDP_EL);
	rtsp[cur_conn_num]->payload_type=96;
	//sprintf(sdp_buff + strlen(sdp_buff), "%d"SDP_EL, rtsp[cur_conn_num]->payload_type);
	if (rtsp[cur_conn_num]->payload_type>=96) {
		/**** Dynamically defined payload ****/
		strcat(sdp_buff,"a=rtpmap:");
		sprintf(sdp_buff + strlen(sdp_buff), "%d", rtsp[cur_conn_num]->payload_type);
		strcat(sdp_buff," ");	
		strcat(sdp_buff,"H264/90000");
		strcat(sdp_buff, SDP_EL);

#if 1
		
		strcat(sdp_buff,"a=fmtp:96 packetization-mode=1;profile-level-id=64001F;sprop-parameter-sets=Z0IAKp2oHgCJ+WbgICAgQA==,aM48gA==");

		//strcat(sdp_buff,"a=fmtp:96 packetization-mode=1;profile-level-id=64001F;sprop-parameter-sets=Z2QAMq2EBUViuKxUdCAqKxXFYqOhAVFYrisVHQgKisVxWKjoQFRWK4rFR0ICorFcVio6ECSFITk8nyfk/k/J8nm5s00IEkKQnJ5Pk/J/J+T5PNzZprQBAAYNKkAAAB4AAASwGBAABMS0AAFXUu974XhEI1AAAAAB,aO48sA==");
#else
		//printf("----->%d  \n",__LINE__);

        //if(sps_base == NULL){
              global_sps = 0;
	    //}

		if(global_sps == 0){
			memset(sps_data,0,544);
			//printf("----->%d  \n",__LINE__);	
	    	get_sps_pps(sps_data,&sps_len);
            //printf("----->%d  \n",__LINE__);	
			memset(sps_base,0,544);		
			sps_ret = b64_encode(sps_base,544,sps_data,sps_len);
		}
		//global_sps = 1; 
           
		/*
        for(i = 0;i < sps_len;i++)
			printf("%02x",sps_data[i]);
		printf("\n");
		for(i = 0;i < pps_len;i++)
			printf("%02x",pps_data[i]);
		printf("\n");
		*/
		sprintf(sdp_buff + strlen(sdp_buff),"a=fmtp:96 packetization-mode=1;profile-level-id=64001F;sprop-parameter-sets=%s",sps_base);
	    //printf("sps_base:%s\n",sps_base);
		//snprintf(sdp_buff + strlen(sdp_buff),pps_len*2 ,"%s",pps_base);
		printf("pps_base:%s\n",sps_base);
#endif		
		strcat(sdp_buff, SDP_EL);
		strcat(sdp_buff,"a=control:track1");
		strcat(sdp_buff, SDP_EL);


		audio_encode_type = 1;
		audio_enable = 0;
		if(audio_enable == 1){
			// audio describe for g711 pcmu 
			if(audio_encode_type == 0){
				strcat(sdp_buff,"m=audio 0 RTP/AVP 0");
				strcat(sdp_buff, SDP_EL);
		        strcat(sdp_buff,"c=IN IP4 0.0.0.0");
				strcat(sdp_buff, SDP_EL);
				strcat(sdp_buff,"b=AS:64");
				strcat(sdp_buff, SDP_EL);
		        strcat(sdp_buff,"a=control:track2");
			}
			else{
				//audio describe for AAC-HR
				strcat(sdp_buff,"m=audio 0 RTP/AVP 96");
				strcat(sdp_buff, SDP_EL);
		        strcat(sdp_buff,"c=IN IP4 0.0.0.0");
				strcat(sdp_buff, SDP_EL);
				strcat(sdp_buff,"b=AS:64");
				strcat(sdp_buff, SDP_EL);
				strcat(sdp_buff,"a=rtpmap:96 MPEG4-GENERIC/8000");
				strcat(sdp_buff, SDP_EL);	
				strcat(sdp_buff,"a=fmtp:96 streamtype=5;profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1588");
				strcat(sdp_buff, SDP_EL);
		        strcat(sdp_buff,"a=control:track2");
			}
		}
		strcat(sdp_buff, SDP_EL);

	}
	return 0;

}
#endif

/******************************************************************************/
 /*
  *  describe command processing functions 
  * Arguments:
  *     cur_conn_num :    current connect number
  */
 /******************************************************************************/
S32 rtsp_describe(S32 cur_conn_num){

    int port = 0;
	int video_type =0;
	if(!rtsp[cur_conn_num]->in_buffer)
		return -1;
	
	check_rtsp_url(cur_conn_num);
	
    printf("=====ch %d,media name :%s \n",cur_conn_num,rtsp[cur_conn_num]->file_name);
	if((atoi(rtsp[cur_conn_num]->file_name) != 0) && (atoi(rtsp[cur_conn_num]->file_name) != 1)){
        send_reply(503,cur_conn_num);
	}
	if(check_rtsp_filename(cur_conn_num)<0)
		return -1;
	// Disallow Header REQUIRE
	if (strstr(rtsp[cur_conn_num]->in_buffer, HDR_REQUIRE)) {
		send_reply(551,cur_conn_num);	/* Option not supported */
		return -1;
	}
	/* Get the description format. SDP is recomended */
	if (strstr(rtsp[cur_conn_num]->in_buffer, HDR_ACCEPT) != NULL) {
		if (strstr(rtsp[cur_conn_num]->in_buffer, "application/sdp") != NULL) {
			//descr_format = df_SDP_format;
			
		} else {
			// Add here new description formats
			send_reply(551,cur_conn_num);	/* Option not supported */
			return -1;
		}
	}
	if(get_rtsp_cseg(cur_conn_num)<0)
		return -1;

    printf("===== __%d__ ==== port:%d \n",__LINE__,port);
	
	if(get_describe_sdp(rtsp[cur_conn_num]->sdp_buffer,cur_conn_num,video_type)<0){
		return -1;
	}
	if(send_describe_reply(200,cur_conn_num)!=-1)
		return 1;


	return 0;
}


