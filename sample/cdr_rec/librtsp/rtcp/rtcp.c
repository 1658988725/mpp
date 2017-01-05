
#include "rtcp.h"
#include "../rtsp/rtsp.h"
#include "../comm/type.h"
#include "../rtp/rtp.h"


#if 0
/******************************************************************************/
/*
 *	send rtcp rr packet
 * Arguments: NULL

 */
/******************************************************************************/

S32 bulid_rtcp_rr(struct rtcp_pkt *pkt)
{
	U32 local_ssrc;

	if(!pkt)
		return -1;
	pkt->comm.ver=2;
    pkt->comm.pad=0;
	pkt->comm.count=1;
	pkt->comm.pt = 201;
	pkt->comm.len = htons(pkt->comm.count * 6 + 1);
	local_ssrc = random32(0);
	pkt->revc_port.ssrc = htonl(local_ssrc);

	return (pkt->comm.count * 6 + 2);

}


/******************************************************************************/
/*
 *	send rtcp sr packet
 * Arguments: NULL

 */
/******************************************************************************/

S32 bulid_rtcp_sr(struct rtcp_pkt *pkt)
{
	U32 local_ssrc;

	if(!pkt)
		return -1;
	pkt->comm.ver=2;
    pkt->comm.pad=0;
	pkt->comm.count=1;
	pkt->comm.pt = 200;
	pkt->comm.len = htons(pkt->comm.count * 6 + 1);
	local_ssrc = random32(0);
	pkt->revc_port.ssrc = htonl(local_ssrc);

	return (pkt->comm.count * 6 + 2);

}
#endif
/******************************************************************************/
/*
 *	send rtcp  packet
 * Arguments: NULL

 */
/******************************************************************************/
S32 rtcp_tcp_send_packet(CHAR *rtcp_ptr,S32 cur_conn,S32 video_flag)
{
	//U8 *pkt=NULL;
	static rtp_frame_t rtp_frame = {0x24,0x00,0x00};
	rtcp_pkt_1 pkt1;
	rtcp_pkt_2 pkt2;
	
	struct timeval tv;
	uint32_t tmp;
	gettimeofday(&tv,NULL);
	tmp=(uint32_t)((double)tv.tv_usec*(double)(1LL<<32)*1.0e-6);
	RTCP_header hdr;	
	RTCP_header_SDES hdr_sdes;
	U32 pkt_size=0,hdr_s=0,hdr_sdes_s,name_s;

	pkt1.comm.version=2;
	pkt1.comm.padding=0;
	pkt1.comm.count=0;
	pkt1.comm.pt=SR;

	pkt1.comm.length = htons(0x06);

	if(video_flag == 1){
		pkt1.ssrc = htonl(rtsp[cur_conn]->cmd_port.ssrc);
		pkt1.sdec.ntp_timestamp_msw = htonl(tv.tv_sec + 0x83AA7E80);
        pkt1.sdec.ntp_timestamp_lsw = htonl(tmp); /* 0x83AA7E80 is the number of seconds from 1900 to 1970 */
		
		pkt1.sdec.rtp_timestamp  = htonl(rtsp[cur_conn]->cmd_port.timestamp);
        pkt1.sdec.senders_packet_count = htonl(rtsp[cur_conn]->cmd_port.video_packet_count);
		//printf("video_packet_count:%d\n",rtsp[cur_conn]->cmd_port.video_packet_count);
        pkt1.sdec.senders_octet_count  = htonl(rtsp[cur_conn]->cmd_port.video_octet_count);
	}else{
        pkt1.ssrc = htonl(rtsp[cur_conn]->cmd_port.audio_ssrc);
		pkt1.sdec.ntp_timestamp_msw = htonl(tv.tv_sec + 0x83AA7E80);
        pkt1.sdec.ntp_timestamp_lsw = htonl(tmp); /* 0x83AA7E80 is the number of seconds from 1900 to 1970 */
		pkt1.sdec.rtp_timestamp = htonl(rtsp[cur_conn]->cmd_port.audio_timestamp);
        pkt1.sdec.senders_packet_count = htonl(rtsp[cur_conn]->cmd_port.audio_packet_count);
        pkt1.sdec.senders_octet_count  = htonl(rtsp[cur_conn]->cmd_port.audio_octet_count);
	}

	//printf(" rtcp_pkt_1 len = %d\n",sizeof(rtcp_pkt_1));
	//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    memcpy(rtcp_ptr+RTP_MAGIC_SIZE,&pkt1,sizeof(rtcp_pkt_1));

	pkt2.comm.version=2;
	pkt2.comm.padding=0;
	pkt2.comm.count=1;
	pkt2.comm.pt=SDES;
	hdr_s=sizeof(hdr);
	hdr_sdes_s=sizeof(hdr_sdes);
	name_s=strlen(rtsp[0]->host_name);
	//pkt_size=hdr_s+hdr_sdes_s+name_s;
	pkt_size = sizeof(rtcp_pkt_2) + name_s;
	pkt2.comm.length=htons((pkt_size >> 2) -1);
	
	//pkt2.sdec.ssrc = htonl(local_ssrc); 
	if(video_flag == 1){
		pkt2.sdec.ssrc = htonl(rtsp[cur_conn]->cmd_port.ssrc);
	}else{
        pkt2.sdec.ssrc = htonl(rtsp[cur_conn]->cmd_port.audio_ssrc);
	}
	pkt2.sdec.attr_name=CNAME;	// 1=CNAME
	pkt2.sdec.len=name_s;
	
	//memcpy(pkt2.sdec.name, rtsp[0]->host_name, name_s);
	//printf("---host_name = %s, len %d\n", rtsp[0]->host_name,name_s);
	//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
	//去掉memcpy,否则出现memcpy错误
	//memcpy(pkt2.sdec.name, rtsp[0]->host_name, name_s);
	//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    if(video_flag==1)
    {
        rtp_frame.channel = rtsp[cur_conn]->cmd_port.rtcp_channel_track1_id;
    }
    else
    {
        rtp_frame.channel = rtsp[cur_conn]->cmd_port.rtcp_channel_track2_id;
    }
    
    rtp_frame.leng = htons((unsigned short)(sizeof(rtcp_pkt_1) + pkt_size-1));
	//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    memcpy(rtcp_ptr,&rtp_frame,sizeof(rtp_frame_t)); 
    //printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    memcpy(rtcp_ptr + RTP_MAGIC_SIZE + sizeof(rtcp_pkt_1),&pkt2,pkt_size-1);

	return (sizeof(rtcp_pkt_1) + pkt_size -1 + 4);

}



/******************************************************************************/
/*
 *	send rtcp  packet
 * Arguments: NULL

 */
/******************************************************************************/
S32 rtcp_send_packet(CHAR *rtcp_ptr,S32 cur_conn,S32 video_flag)
{
	//U8 *pkt=NULL;
	rtcp_pkt_1 pkt1;
	rtcp_pkt_2 pkt2;
	
	struct timeval tv;
	uint32_t tmp;
	gettimeofday(&tv,NULL);
	tmp=(uint32_t)((double)tv.tv_usec*(double)(1LL<<32)*1.0e-6);
	RTCP_header hdr;	
	RTCP_header_SDES hdr_sdes;
	U32 pkt_size=0,hdr_s=0,hdr_sdes_s,name_s;

	pkt1.comm.version=2;
	pkt1.comm.padding=0;
	pkt1.comm.count=0;
	pkt1.comm.pt=SR;

	pkt1.comm.length = htons(0x06);

	if(video_flag == 1){
		pkt1.ssrc = htonl(rtsp[cur_conn]->cmd_port.ssrc);
		pkt1.sdec.ntp_timestamp_msw = htonl(tv.tv_sec + 0x83AA7E80);
        pkt1.sdec.ntp_timestamp_lsw = htonl(tmp); /* 0x83AA7E80 is the number of seconds from 1900 to 1970 */
		
		pkt1.sdec.rtp_timestamp  = htonl(rtsp[cur_conn]->cmd_port.timestamp);
        pkt1.sdec.senders_packet_count = htonl(rtsp[cur_conn]->cmd_port.video_packet_count);
		//printf("video_packet_count:%d\n",rtsp[cur_conn]->cmd_port.video_packet_count);
        pkt1.sdec.senders_octet_count  = htonl(rtsp[cur_conn]->cmd_port.video_octet_count);
	}else{
        pkt1.ssrc = htonl(rtsp[cur_conn]->cmd_port.audio_ssrc);
		pkt1.sdec.ntp_timestamp_msw = htonl(tv.tv_sec + 0x83AA7E80);
        pkt1.sdec.ntp_timestamp_lsw = htonl(tmp); /* 0x83AA7E80 is the number of seconds from 1900 to 1970 */
		pkt1.sdec.rtp_timestamp = htonl(rtsp[cur_conn]->cmd_port.audio_timestamp);
        pkt1.sdec.senders_packet_count = htonl(rtsp[cur_conn]->cmd_port.audio_packet_count);
        pkt1.sdec.senders_octet_count  = htonl(rtsp[cur_conn]->cmd_port.audio_octet_count);
	}
    //printf("%s----->%d  \n", __FUNCTION__, __LINE__);
	memcpy(rtcp_ptr,&pkt1,sizeof(rtcp_pkt_1));
	//printf(" rtcp_pkt_1 len = %d\n",sizeof(rtcp_pkt_1));

	pkt2.comm.version=2;
	pkt2.comm.padding=0;
	pkt2.comm.count=1;
	pkt2.comm.pt=SDES;
	hdr_s=sizeof(hdr);
	hdr_sdes_s=sizeof(hdr_sdes);
	name_s=strlen(rtsp[0]->host_name);
	//pkt_size=hdr_s+hdr_sdes_s+name_s;
	pkt_size = sizeof(rtcp_pkt_2) + name_s;
	pkt2.comm.length=htons((pkt_size >> 2) -1);
	
	//pkt2.sdec.ssrc = htonl(local_ssrc); 
	if(video_flag == 1){
		pkt2.sdec.ssrc = htonl(rtsp[cur_conn]->cmd_port.ssrc);
	}else{
        pkt2.sdec.ssrc = htonl(rtsp[cur_conn]->cmd_port.audio_ssrc);
	}
	pkt2.sdec.attr_name=CNAME;	// 1=CNAME
	pkt2.sdec.len=name_s;
	//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
	//去掉memcpy，否则会出现memcpy错误
	//memcpy(pkt2.sdec.name, rtsp[0]->host_name, name_s+1);
	//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    memcpy(rtcp_ptr + sizeof(rtcp_pkt_1),&pkt2,pkt_size-1);
	//printf("pkg2 len = %d",pkt_size);
	
	//return pkt_size;
	return (sizeof(rtcp_pkt_1) + pkt_size-1);

}



