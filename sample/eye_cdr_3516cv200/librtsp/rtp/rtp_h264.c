/*******
 * 
 * FILE INFO: proc rtp h264  packet
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2011/04/25  09:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2011/04/28 20:18:24
 * updated by:	
 * version: 1.0.0.0
 * 
 *******/
#include "rtp.h"

#include "rtcp.h"
#include "rtsp.h"
#include "type.h"
#include "OSAL_Queue.h"

#include <sys/time.h>
//extern QBuff mBuf[NB_BUFFER];
extern OSAL_QUEUE		mQueueBuffer;

//#define WRITE_FILE 1
int audio_encode_type;
int audio_enable;


/******************************************************************************/
/*
 *  get media file  size
 * Arguments: 
 * infile : file name fd
 */
/******************************************************************************/
L64 get_file_size(FILE *infile)
{
    L64 size_of_file;
	
	/****  jump to the end of the file. ****/
    fseek(infile,0L,SEEK_END);
	/****  get the end position. ****/
	size_of_file = ftell(infile);
    /**** Jump back to the original position. ****/
    fseek(infile, 0L,SEEK_SET );
	return (size_of_file);
}

/******************************************************************************/
/*
 *  get media file  size
 * Arguments: 
 * infile : file name fd
 */
/******************************************************************************/
S32 wirte_to_file(U8 *buff,S32 buffer_size,FILE *outfile)
{
	L64 nbytes_read;

	nbytes_read = fwrite(buff, 1, buffer_size,outfile);
	return nbytes_read;
}




/******************************************************************************/
/*
 *  get buffer len
 * Arguments: 
 * str :  input buffer
 */
/******************************************************************************/
S32 my_strlen(const CHAR * str)
{
	S32 ret=0;
    const CHAR * p=str;
	
    if(NULL==str) return -1;

	while(*p++){
		ret++; 
	}
    return ret;
}

/******************************************************************************/
/*
 * create ran  seq number 
 * Arguments: 
 * 
 */
/******************************************************************************/
UL64 get_randdom_seq(VOID)
{
	UL64 seed;
	srand((unsigned)time(NULL));  
	seed = 1 + (U32) (rand()%(0xFFFF));	
	
	return seed;
}



/******************************************************************************/
/*
 * create ran  timestamp number 
 * Arguments: 
 * 
 */
/******************************************************************************/
UL64 get_randdom_timestamp(VOID)
{
	UL64 seed;
	srand((unsigned)time(NULL));  
	seed = 1 + (U32) (rand()%(0xFFFFFFFF));	
	
	return seed;
}


/******************************************************************************/
/*
 * use  gettimeofday  get  timestamp 
 * Arguments: 
 * 
 */
/******************************************************************************/
L64  get_timestamp()
{
    struct timeval tv_date;

    /* gettimeofday() could return an error, and should be tested. However, the
     * only possible error, according to 'man', is EFAULT, which can not happen
     * here, since tv is a local variable. */
    gettimeofday( &tv_date, NULL );
    return( (L64 ) tv_date.tv_sec * 1000000 + (L64 ) tv_date.tv_usec );
}



/******************************************************************************/
/*
 *  build h264  frame header 
 * Arguments: 
 * RTP_header:  rtp header strcut
 *	  cur_conn_num :	current connect number
 */
/******************************************************************************/
S32 build_rtp_header(RTP_header *r,S32 cur_conn_num,int audio_flag,int audio_type,U32 audio_time,U32 video_time)
{	
	r->version = 2;
	r->padding = 0;
	r->extension = 0;
	r->csrc_len = 0;
	r->marker=0;
	r->payload=96;
	if(audio_flag == 0){
        
    	r->seq_no=htons(rtsp[cur_conn_num]->cmd_port.seq);
    	//rtsp[cur_conn_num]->cmd_port.timestamp+=rtsp[cur_conn_num]->cmd_port.frame_rate_step;
    	r->timestamp=htonl(video_time);
		rtsp[cur_conn_num]->cmd_port.timestamp = video_time;
    	r->ssrc = htonl(rtsp[cur_conn_num]->cmd_port.ssrc);
	}else if((audio_flag == 1) && (audio_type == 1)){
        
    	//audio type is  aac  
    	r->seq_no = htons(rtsp[cur_conn_num]->cmd_port.audio_seq);
    	rtsp[cur_conn_num]->cmd_port.audio_timestamp += 1024;
    	r->timestamp = htonl(rtsp[cur_conn_num]->cmd_port.audio_timestamp);
    	r->ssrc = htonl(rtsp[cur_conn_num]->cmd_port.audio_ssrc);
	}else if((audio_flag == 1) && (audio_type == 0)){

		//audio type is g711  
    	r->payload=0;
    	r->seq_no=htons(rtsp[cur_conn_num]->cmd_port.audio_seq);
    	rtsp[cur_conn_num]->cmd_port.audio_timestamp+=1024;
    	r->timestamp=htonl(rtsp[cur_conn_num]->cmd_port.audio_timestamp);
    	r->ssrc = htonl(rtsp[cur_conn_num]->cmd_port.audio_ssrc);
	}
	
	return 0;
}

/******************************************************************************/
/*
 *Write "n" bytes to a descriptor. 
 * Arguments: 
 * fd:  write buffer fd
 * vptr: write  buffer 
 * n: write buffer len
 */
/******************************************************************************/
ssize_t write_n(S32 fd, const VOID *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const CHAR	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;		/**** and call write() again ****/
			else
				return(-1);			/**** error ****/
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
S32 rtp_tcp_write(S32 len,S32 cur_conn_num,S32 audio_flag)
{
	S32 result;
	int exit_flag = 0;
	if(rtsp[cur_conn_num]->is_sending_data == 1)
	{
    	if(audio_flag == 0){
            //result = write(rtsp[cur_conn_num]->fd.video_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len);
            result = send_tcp_pkg(rtsp[cur_conn_num]->fd.video_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len,0,&exit_flag);
			if(result == 0)
			{
				rtsp[cur_conn_num]->timeout_count++;
                printf("ch: %d, ip: %s, send_tcp_pkg video return 0...count = %d\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host, rtsp[cur_conn_num]->timeout_count);
                if(rtsp[cur_conn_num]->timeout_count > 7)
					result = -1;
				else
				    return 1;
			}
			rtsp[cur_conn_num]->timeout_count = 0;
			if(result > 0 && result != len)
			{
                printf("chn: %d, ip: %s, sucess send video data %d, miss %d\n",cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host, result, len - result);
			}
    	    else if(result < 0){
    			sem_wait(&rtspd_semop_sending_data);
    			rtsp_tcp_para_clear(cur_conn_num);
    			sem_post(&rtspd_semop_sending_data);
                
    			printf("%s, %d, chn %d, ip : %s\n", __func__, __LINE__, cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
    			perror("rtsp video packet send error");
    			return -1;
    	    }
			
    		rtsp[cur_conn_num]->cmd_port.video_packet_count++;
    		rtsp[cur_conn_num]->cmd_port.video_octet_count += result;
    	}
        else if(rtsp[cur_conn_num]->cmd_port.rtp_channel_track2_id != -1){
            //result = write(rtsp[cur_conn_num]->fd.audio_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len);
            result = send_tcp_pkg(rtsp[cur_conn_num]->fd.video_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len,0,&exit_flag);
			if(result == 0)
			{
				rtsp[cur_conn_num]->timeout_count++;
                printf("ch: %d, ip: %s, send_tcp_pkg audio return 0...count = %d\n", cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host, rtsp[cur_conn_num]->timeout_count);
                if(rtsp[cur_conn_num]->timeout_count > 7)
					result = -1;
				else
				    return 1;
			}
			rtsp[cur_conn_num]->timeout_count = 0;
			if(result > 0 && result != len)
			{
                printf("chn: %d, ip: %s, sucess send audio data %d, miss %d\n",cur_conn_num,rtsp[cur_conn_num]->cli_rtsp.cli_host, result, len - result);
			}
    	    else if(result < 0){
    			sem_wait(&rtspd_semop_sending_data);
    			rtsp_tcp_para_clear(cur_conn_num);
    			sem_post(&rtspd_semop_sending_data);
                
    			printf("%s, %d, chn %d, ip : %s\n", __func__, __LINE__, cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
    			perror("rtsp audio packet send error");
    			return -1;

    	    }
    		rtsp[cur_conn_num]->cmd_port.audio_packet_count++;
    		rtsp[cur_conn_num]->cmd_port.audio_octet_count += result;            
    	}
        else
        {
            return 0;//-1;
        }
	}
	else
		return -1;
	

	return 0;
}

/******************************************************************************/
/*
 * Use the udp protocol 
 * Arguments: 
 * RTP_header:  rtp header strcut
 * len: udp buffer len
 *	  cur_conn_num :	current connect number
 */
/******************************************************************************/
S32 udp_write(S32 len,S32 cur_conn_num,S32 audio_flag)
{
	S32 result;
	if(rtsp[cur_conn_num]->is_sending_data == 1)
	{
    	if(audio_flag == 0){
            result = write(rtsp[cur_conn_num]->fd.video_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len);
			if(result > 0 && result != len)
			{
                printf("chn: %d, ip: %s, sucess send video data %d, miss %d\n",cur_conn_num,rtsp[cur_conn_num]->cli_rtsp.cli_host, result, len - result);
			}
    	    if(result <=0){
    			sem_wait(&rtspd_semop_sending_data);
				rtsp_tcp_para_clear(cur_conn_num);
    			sem_post(&rtspd_semop_sending_data);
    			printf("%s, %d, chn %d, ip : %s\n", __func__, __LINE__, cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
    			perror("rtsp video packet send error");
    			return -1;
    	    }
    		rtsp[cur_conn_num]->cmd_port.video_packet_count++;
    		rtsp[cur_conn_num]->cmd_port.video_octet_count += result;
    	}
        else if(rtsp[cur_conn_num]->cmd_port.rtp_udp_track2_port != -1)
    	{
    	
            result = write(rtsp[cur_conn_num]->fd.audio_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len);
			if(result > 0 && result != len)
			{
                printf("chn: %d, ip: %s, sucess send audio data %d, miss %d\n",cur_conn_num,rtsp[cur_conn_num]->cli_rtsp.cli_host, result, len - result);
			}

    	    if(result<=0){
    			sem_wait(&rtspd_semop_sending_data);
				rtsp_tcp_para_clear(cur_conn_num);
    			sem_post(&rtspd_semop_sending_data);
    			printf("%s, %d, chn %d, ip : %s\n", __func__, __LINE__, cur_conn_num, rtsp[cur_conn_num]->cli_rtsp.cli_host);
    			perror("rtsp audio packet send error");
    			return -1;

    	    }
    		rtsp[cur_conn_num]->cmd_port.audio_packet_count++;
    		rtsp[cur_conn_num]->cmd_port.audio_octet_count += result;
    	}
        else
        {
            return 0;//-1;
        }
	}
	

	return 0;
}


/******************************************************************************/
/*
 * Use the udp protocol 
 * Arguments: 
 * RTP_header:  rtp header strcut
 * len: udp buffer len
 * time: delayer time
 *	  cur_conn_num :	current connect number
 */
/******************************************************************************/
S32 udp_write_fua(S32 len,S32 time,S32 cur_conn_num)
{
	S32 result;
	again:
	result=write(rtsp[cur_conn_num]->fd.video_rtp_fd,rtsp[cur_conn_num]->nalu_buffer,len);
	if(result<=0){
		goto again;
	}
	else{
		if(time>DE_TIME)
			time=DE_TIME;
		usleep(time);
	}
	return 0;
}

/*******************************************************************************
 * Describ : Abstract the NALU Indicator bytes(00 00 00 01);
 * Argument: buf -- input, Data buffer,
 *           buf_size -- input, Buffer size.
 *           be_found -- output, set to 1 if found Indicator.
 * return  : If found, return frame_size;
 *           others return buf_size.
 */
S32 abstr_nalu_indic(U8 *buf, S32 buf_size, S32 *be_found)
{
    U8 *p_tmp;
	S32 offset;
	S32 frame_size;
	
	*be_found = 0;
	offset = 0;
	frame_size = 4;	
	p_tmp = buf + 4;
	
	while(frame_size < buf_size - 4)
	{
	    if(p_tmp[2])
			offset = 3;
		else if(p_tmp[1])
			offset = 2;
		else if(p_tmp[0])
			offset = 1;
		else
		{
		    if(p_tmp[3] != 1)
		    {
		        if(p_tmp[3])
					offset = 4;
				else
					offset = 1;
		    }
			else
			{
			    *be_found = 1;
				break;
			}
		}

		frame_size += offset;
		p_tmp += offset;
	}

	if(!*be_found)
		frame_size = buf_size;

	return frame_size;
}
/*******************************************************************************
 * RTP Packet:
 * 1. NALU length small than 1460-sizeof(RTP header):
 *    (RTP Header) + (NALU without Start Code)
 * 2. NALU length larger than MTU:
 *    (RTP Header) + (FU Indicator) + (FU Header) + (NALU Slice)
 *                 + (FU Indicator) + (FU Header) + (NALU Slice)
 *                 + ...
 *
 * inbuffer--NALU: 00 00 00 01      1 Byte     XX XX XX
 *                | Start Code| |NALU Header| |NALU Data|
 *
 * NALU Slice: Cut NALU Data into Slice.
 *
 * NALU Header: F|NRI|TYPE
 *              F: 1 bit,
 *              NRI: 2 bits,
 *              Type: 5 bits
 *
 * FU Indicator: Like NALU Header, Type is FU-A(28)
 * 
 * FU Header: S|E|R|Type
 *            S: 1 bit, Start, First Slice should set
 *            E: 1 bit, End, Last Slice should set
 *            R: 1 bit, Reserved
 *            Type: 5 bits, Same with NALU Header's Type.
 ******************************************************************************/
S32 build_rtp_tcp_nalu(U8 *inbuffer, S32 frame_size, S32 cur_conn_num,S32 audio_flag,S32 audio_type,U32 audio_time,U32 video_time)
{
    RTP_header rtp_header;
    S32 time_delay;
    S32 data_left;
    
    U8 nalu_header;
    U8 fu_indic;
    U8 fu_header;   
    U8 *p_nalu_data;
    U8 *nalu_buffer;

    static rtp_frame_t rtp_frame = {0x24,0x00,0x00};
    static char aac_packet[4] = {0x00,0x10,0x00,0x00};
    aac_packet[2] = ((unsigned int)frame_size & 0x1fe0) >> 5;
    aac_packet[3] = ((unsigned int)frame_size & 0x1f) << 3;
    
    S32 fu_start = 1;
    S32 fu_end   = 0;
    S32 ret = -1;
    if(!inbuffer)
        return -1;

    nalu_buffer = rtsp[cur_conn_num]->nalu_buffer;
    
    build_rtp_header(&rtp_header,cur_conn_num,audio_flag,audio_type,audio_time,video_time);

    if(audio_flag == 0){ //h264 video stream
        data_left   = frame_size - NALU_INDIC_SIZE;
        p_nalu_data = inbuffer + NALU_INDIC_SIZE;

    }else{ //audio stream
        data_left   = frame_size;
        p_nalu_data = inbuffer; 
    }
    
    int  rtp_magic_len = sizeof(rtp_frame_t);
    
    //Single RTP Packet.
    if(data_left <= SINGLE_TCP_NALU_DATA_MAX)
    {
        if(audio_flag == 0){
			//h264
            rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.seq++);
            rtp_header.marker=1;
            rtp_frame.channel = rtsp[cur_conn_num]->cmd_port.rtp_channel_track1_id;
            rtp_frame.leng = htons((unsigned short)(data_left+RTP_HEADER_SIZE)); 
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer,&rtp_frame,sizeof(rtp_frame_t));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_MAGIC_SIZE ,&rtp_header,sizeof(rtp_header));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_MAGIC_SIZE+ RTP_HEADER_SIZE , p_nalu_data, data_left);
            ret = rtp_tcp_write(data_left + RTP_MAGIC_SIZE+ RTP_HEADER_SIZE , cur_conn_num,audio_flag);
            return ret; 
        }else if((audio_flag == 1) && (audio_type == 1)){
            //audio type is aac 
            rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.audio_seq++);
            rtp_header.marker=1;
            rtp_frame.channel = rtsp[cur_conn_num]->cmd_port.rtp_channel_track2_id;
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            rtp_frame.leng = htons((unsigned short)(data_left+RTP_HEADER_SIZE+4));
            memcpy(nalu_buffer,&rtp_frame,sizeof(rtp_frame_t));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_MAGIC_SIZE,&rtp_header,sizeof(rtp_header));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_MAGIC_SIZE + sizeof(rtp_header) ,aac_packet,4);
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_MAGIC_SIZE+ RTP_HEADER_SIZE + 4, p_nalu_data, data_left);
            ret = rtp_tcp_write(data_left + RTP_MAGIC_SIZE + RTP_HEADER_SIZE + 4, cur_conn_num,audio_flag);
            return ret;
        }else if((audio_flag == 1) && (audio_type == 0)){
            //audio type is g711 pcmu
            rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.audio_seq++);
            rtp_header.marker=0;
            rtp_frame.channel = rtsp[cur_conn_num]->cmd_port.rtp_channel_track2_id;
            rtp_frame.leng = htons((unsigned short)(data_left+RTP_HEADER_SIZE)); 
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
			memcpy(nalu_buffer,&rtp_frame,sizeof(rtp_frame_t));    
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_MAGIC_SIZE,&rtp_header,sizeof(rtp_header));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
            memcpy(nalu_buffer + RTP_HEADER_SIZE + RTP_MAGIC_SIZE, p_nalu_data, data_left);
            ret = rtp_tcp_write(data_left + RTP_HEADER_SIZE + RTP_MAGIC_SIZE , cur_conn_num,audio_flag);
            return ret;
        }
        return 0;
    }

    //FU-A RTP Packet.
    nalu_header = inbuffer[4];
    fu_indic    = (nalu_header&0xE0)|28;    
    data_left   -= NALU_HEAD_SIZE;
    p_nalu_data += NALU_HEAD_SIZE;
    while(data_left>0)
    {
        S32 proc_size = MIN(data_left,SLICE_NALU_TCP_DATA_MAX);
        S32 rtp_size = proc_size + 
                RTP_MAGIC_SIZE   +
                RTP_HEADER_SIZE  + 
                FU_A_HEAD_SIZE   + 
                FU_A_INDI_SIZE   ;
        fu_end = (proc_size==data_left);
        fu_header = nalu_header&0x1F;
        if(fu_start)
            fu_header |= 0x80;
        else if(fu_end){
            fu_header |= 0x40;
            rtp_header.marker = 1;
        }

        rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.seq++);
        rtp_frame.channel = rtsp[cur_conn_num]->cmd_port.rtp_channel_track1_id;
        rtp_frame.leng = htons((unsigned short)(rtp_size-RTP_MAGIC_SIZE));
		//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
        memcpy(nalu_buffer,&rtp_frame,sizeof(rtp_frame_t));   
		//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
        memcpy(nalu_buffer + RTP_MAGIC_SIZE,&rtp_header,sizeof(rtp_header));
		//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
        memcpy(nalu_buffer + 14 + RTP_MAGIC_SIZE,p_nalu_data,proc_size);
        nalu_buffer[12+RTP_MAGIC_SIZE] = fu_indic;
        nalu_buffer[13+RTP_MAGIC_SIZE] = fu_header;
        ret = rtp_tcp_write(rtp_size, cur_conn_num,audio_flag);
		if(ret != 0)
            return ret;
        //if(fu_end)
        //  usleep(36000);
        
        data_left -= proc_size; 
        p_nalu_data += proc_size;
        fu_start = 0;
    }

    return 0;   
}



/*******************************************************************************
 * RTP Packet:
 * 1. NALU length small than 1460-sizeof(RTP header):
 *    (RTP Header) + (NALU without Start Code)
 * 2. NALU length larger than MTU:
 *    (RTP Header) + (FU Indicator) + (FU Header) + (NALU Slice)
 *                 + (FU Indicator) + (FU Header) + (NALU Slice)
 *                 + ...
 *
 * inbuffer--NALU: 00 00 00 01      1 Byte     XX XX XX
 *                | Start Code| |NALU Header| |NALU Data|
 *
 * NALU Slice: Cut NALU Data into Slice.
 *
 * NALU Header: F|NRI|TYPE
 *              F: 1 bit,
 *              NRI: 2 bits,
 *              Type: 5 bits
 *
 * FU Indicator: Like NALU Header, Type is FU-A(28)
 * 
 * FU Header: S|E|R|Type
 *            S: 1 bit, Start, First Slice should set
 *            E: 1 bit, End, Last Slice should set
 *            R: 1 bit, Reserved
 *            Type: 5 bits, Same with NALU Header's Type.
 ******************************************************************************/
S32 build_rtp_nalu(U8 *inbuffer, S32 frame_size, S32 cur_conn_num,S32 audio_flag,S32 audio_type,U32 audio_time,U32 video_time)
{
	RTP_header rtp_header;
	S32 time_delay;
	S32 data_left;
	
	U8 nalu_header;
	U8 fu_indic;
	U8 fu_header;	
	U8 *p_nalu_data;
	U8 *nalu_buffer;

	//static rtp_frame_t rtp_frame = {0x24,0x00,0x00};
	static char aac_packet[4] = {0x00,0x10,0x00,0x00};
	aac_packet[2] = ((unsigned int)frame_size & 0x1fe0) >> 5;
	aac_packet[3] = ((unsigned int)frame_size & 0x1f) << 3;
		
	S32 fu_start = 1;
	S32 fu_end   = 0;
	
    if(!inbuffer)
		return -1;

    nalu_buffer = rtsp[cur_conn_num]->nalu_buffer;
	
	build_rtp_header(&rtp_header,cur_conn_num,audio_flag,audio_type,audio_time,video_time);

	if(audio_flag == 0){
        data_left   = frame_size - NALU_INDIC_SIZE;
		p_nalu_data = inbuffer + NALU_INDIC_SIZE;
		//data_left   = frame_size;
		//p_nalu_data = inbuffer; 
	}else{
        data_left   = frame_size;
		p_nalu_data = inbuffer; 
	}
    int  rtp_magic_len = sizeof(rtp_frame_t);
	//Single RTP Packet.
    if(data_left <= SINGLE_NALU_DATA_MAX)
    {
		if(audio_flag == 0){ //h.264 video stream
            rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.seq++);
			rtp_header.marker=1;
			//rtp_frame.channel = 0;
			//rtp_frame.leng = htons((unsigned short)(data_left));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
			memcpy(nalu_buffer ,&rtp_header,sizeof(rtp_header));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
	        memcpy(nalu_buffer + RTP_HEADER_SIZE , p_nalu_data, data_left);
			if(udp_write(data_left + RTP_HEADER_SIZE , cur_conn_num,audio_flag) < 0)
				return -1;
			//usleep(DE_TIME);
	        return 0;
		}else if((audio_flag == 1) && (audio_type == 1)){ // aac audio stream
			//audio type is aac 
            rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.audio_seq++);
			rtp_header.marker=1;
			//rtp_frame.channel = 2;
			//rtp_frame.leng = htons((unsigned short)(data_left));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
			memcpy(nalu_buffer,&rtp_header,sizeof(rtp_header));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
			memcpy(nalu_buffer + sizeof(rtp_header) ,aac_packet,4);
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
	        memcpy(nalu_buffer + RTP_HEADER_SIZE + 4, p_nalu_data, data_left);
			if(udp_write(data_left + RTP_HEADER_SIZE + 4, cur_conn_num,audio_flag) < 0)
				return -1;
			//usleep(DE_TIME);
	        return 0;
		}else if((audio_flag == 1) && (audio_type == 0)){ //G711 audio stream
			//audio type is g711 pcmu
			rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.audio_seq++);
			rtp_header.marker=0;
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
			memcpy(nalu_buffer,&rtp_header,sizeof(rtp_header));
			//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
	        memcpy(nalu_buffer + RTP_HEADER_SIZE , p_nalu_data, data_left);
			if(udp_write(data_left + RTP_HEADER_SIZE , cur_conn_num,audio_flag) < 0)
				return -1;
			//usleep(DE_TIME);
	        return 0;
		}
	    //memcpy(nalu_buffer,&rtp_frame,sizeof(rtp_frame_t));    
		
    }

	//FU-A RTP Packet.
	nalu_header = inbuffer[4];
	fu_indic    = (nalu_header&0xE0)|28;	
	data_left   -= NALU_HEAD_SIZE;
	p_nalu_data += NALU_HEAD_SIZE;
	while(data_left>0)
	{
	    S32 proc_size = MIN(data_left,SLICE_NALU_DATA_MAX);
		S32 rtp_size = proc_size + 
			    RTP_HEADER_SIZE  + 
			    FU_A_HEAD_SIZE   + 
			    FU_A_INDI_SIZE   ;
		fu_end = (proc_size==data_left);
		fu_header = nalu_header&0x1F;
		if(fu_start)
			fu_header |= 0x80;
		else if(fu_end){
			fu_header |= 0x40;
			rtp_header.marker = 1;
		}

        rtp_header.seq_no=htons(rtsp[cur_conn_num]->cmd_port.seq++);

		//rtp_frame.channel = 0;
		//rtp_frame.leng = htons((unsigned short)(proc_size));
	    //memcpy(nalu_buffer,&rtp_frame,sizeof(rtp_frame_t));    
	    //printf("%s----->%d  \n", __FUNCTION__, __LINE__);
		memcpy(nalu_buffer ,&rtp_header,sizeof(rtp_header));
		//printf("%s----->%d  \n", __FUNCTION__, __LINE__);
		memcpy(nalu_buffer + 14 ,p_nalu_data,proc_size);
		nalu_buffer[12] = fu_indic;
		nalu_buffer[13] = fu_header;
		if(udp_write(rtp_size, cur_conn_num,audio_flag) < 0)
			return -1;
		//if(fu_end)
		//	usleep(36000);
		
		data_left -= proc_size;	
		p_nalu_data += proc_size;
		fu_start = 0;
	}

	return 0;	
}

/******************************************************************************/
/*
 *  from vidoe file  build rtp packet  and send
 *	  cur_conn_num :	current connect number
 */
/******************************************************************************/
S32 rtp_send_form_file(S32 cur_conn_num)
{
	FILE *infile = NULL,*outfile = NULL;
	S32 total_size=0,bytes_consumed=0,frame_size=0, bytes_left;
	U8 inbufs[READ_LEN]="",outbufs[READ_LEN]="";
    U8 *p_tmp = NULL;

	S32 found_nalu = 0;
	S32 reach_last_nalu = 0;
	
    outfile = outfile;
	printf("************%s %s\r\n",__FUNCTION__,rtsp[0]->file_name);
	
	infile = fopen(rtsp[0]->file_name, "rb");
	if(infile==NULL){
		printf("please check media file\n");
		return -1;
	}
	
	total_size=get_file_size(infile);
	if(total_size<=4)
	{
		fclose(infile);
	    return 0;	
	}

	#ifdef WRITE_FILE
	outfile = fopen("test_video.h264", "w");
	if(outfile==NULL){
		printf("please check media file\n");
		return -1;
	}
	#endif

	while(rtsp[cur_conn_num]->is_runing)
	{
	     bytes_left = fread(inbufs,1,READ_LEN,infile);
		 p_tmp = inbufs;
		 while(bytes_left>0)
		 {
		     frame_size = abstr_nalu_indic(p_tmp, bytes_left, &found_nalu);
			 reach_last_nalu = (bytes_consumed + frame_size >= total_size); 

			 if(found_nalu||reach_last_nalu)
			 {	     
			     memcpy(outbufs, p_tmp, frame_size);
				 
				 #ifdef WRITE_FILE
				 wirte_to_file(outbufs, frame_size, outfile);
				 #endif

                 if(rtsp[cur_conn_num]->rtp_tcp_mode==RTP_TCP)
                 {
                    //sem_wait(&rtp_rtcp_lock[cur_conn_num]);
                    build_rtp_tcp_nalu(outbufs, frame_size, cur_conn_num,0,audio_encode_type,0,0);
                    //sem_post(&rtp_rtcp_lock[cur_conn_num]);
                    
                 }
                 else
                 {
                    build_rtp_nalu(outbufs, frame_size, cur_conn_num,0,audio_encode_type,0,0);	
                 }
				 
				 p_tmp += frame_size;
				 bytes_consumed += frame_size;

				 if(reach_last_nalu)
				 	rtsp[cur_conn_num]->is_runing = 0;
			 } 
			 bytes_left -= frame_size;
		 }
	 
	    fseek(infile,bytes_consumed,SEEK_SET);  
	}

    fclose(infile);
	close(rtsp[cur_conn_num]->fd.video_rtp_fd);
	
#ifdef WRITE_FILE
	fclose(outfile);
#endif
	return 0;

}

int AbortSignal = 0;

static void signalHandler(void)
{
	fprintf(stderr,"RTSP Server caught SIGTERM: shutting down\n");
	AbortSignal = 1;
}
 
void handle_signal(int s)
{
    fprintf(stderr, "handle_signal\n");
}



#define LOCK_MP4_VOL					0
#define UNLOCK_MP4_VOL					1
#define LOCK_MP4						2
#define LOCK_MP4_IFRAM					3
#define UNLOCK_MP4						4
#define GET_MPEG4_SERIAL				5
#define WAIT_NEW_MPEG4_SERIAL			6

#define KB   1024
#define NALU_SIZE   256
#define TIME_STAMP_S 90

static char sps_buffer[NALU_SIZE]; //h264 sps data
static char pps_buffer[NALU_SIZE]; //h264 pps data
static char sei_buffer[NALU_SIZE]; //h264 sei data
//static unsigned int sps=0,sps_len=0,pps=0,pps_len=0,sei=0,sei_len=0;
static unsigned int sps=0,sps_len=0,pps=0,pps_len=0,sei=0,sei_len=0;
static unsigned int i_fream_offset = 0; //I fream data nual address
static unsigned int p_fream_offset = 0; //P fream data nual address    	
static int i_nal_status = 0;



/******************************************************************************/
/*
 *   from stream  build rtp packet  and send
 * Arguments: NULL
 
 */
/******************************************************************************/
S32 rtp_send_form_stream(S32 video_type)
{
#if 0
	AbortSignal = 0;
	signal(SIGINT, (VOID*)signalHandler);
	signal(SIGPIPE, handle_signal);
	
    while(AbortSignal == 0)
    {
    	int i = 0;
		int clientCountNow = 0, clientCountOld = 0; 
    	int hasClient = 0;
		int videoCount = 0;
		int audioCount = 0;
		
		printf("wait client..........\n");
        rtsp[0]->V_Serial[0] = 0;
        rtsp[0]->V_Serial[1] = 0;
        rtsp[0]->A_Serial[0] = 0;
        rtsp[0]->A_Serial[1] = 0;
#if 0		
    	while(AbortSignal == 0)
    	{
            if(i >= MAX_CONN )
    			i %= MAX_CONN;
    		if(rtsp[i]->is_sending_data == 1)
    		{
    			hasClient = 1;
    			break;
    		}
			i++;
    		usleep(10000);
    	}
#endif
		if(AbortSignal == 1)
			break;
			
		printf("send begin..........\n");
	
    	int	get_audio_flag = 0;
    	
		printf("found I Frame sucess........\n");

    	char *ptr = NULL;
		int timestamp = 0;
        unsigned int stamp = 0;
		struct timeval _timestamp;
	    struct timezone tz;
		long old_time;
		int send_data_ret = 0;
    	while(AbortSignal == 0)
    	{

			//printf("----->%d  \n",__LINE__);
    		//ret = GetAVData(mpeg4_field[LOCK_MP4][vType], V_SerialBook, &av_data );//get frame
    		QBuff * pbuf = (QBuff *)OSAL_Dequeue(&mQueueBuffer);
			if (pbuf == NULL)
			{
				//printf("dequeue no buffer, sleep...\n");
				usleep(10000);
				continue;
			}
			int j=0;
			unsigned char * tmp = (unsigned char *)pbuf->data;
/*   add for test by chendan
			printf("Dequeue buf size = %d  \n",pbuf->size);
			if(pbuf->size < 100){
				for(j=0; j<pbuf->size; j++){
				    printf("=====  data[%d] = 0x%X  ====\n", j, tmp[j]);
				}			
			}
*/
			for(i = 0; i < MAX_CONN; i++)
			{
				if(rtsp[i]->is_sending_data == 1)
				{	
					if(rtsp[i]->rtp_tcp_mode==RTP_TCP)
					{
						if(rtsp[i]->rtsp_video_stream == 1)
						{					
							send_data_ret = build_rtp_tcp_nalu(pbuf->data,pbuf->size , i,0,audio_encode_type,0,(pbuf->time/1000)*TIME_STAMP_S);//(pbuf->time*TIME_STAMP_S)&0x000000003fffffffll);
						}					                      
					}
					else
					{
						if(rtsp[i]->rtsp_video_stream == 1)
						{
							build_rtp_nalu(pbuf->data ,pbuf->size , i,0,audio_encode_type,0,(pbuf->time/1000)*TIME_STAMP_S);//(pbuf->time*TIME_STAMP_S)&0x000000003fffffffll);
						}				
					}     

				}  
			}			
    	}
        printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    }
#endif
	return 0;
}



/******************************************************************************/
/*
 *   from stream  build rtp packet  and send
 * Arguments: NULL
 
 */
/******************************************************************************/
VOID * rtp_send_form_stream_func(VOID *arg)
{
	AbortSignal = 0;
	signal(SIGINT, (VOID*)signalHandler);
	signal(SIGPIPE, handle_signal);
	pthread_detach(pthread_self());
	S32 nSession = (S32)arg;
	struct rtsp_media_session *pMs = &g_media_session[nSession];
	
    while(AbortSignal == 0)
    {
    	int i = 0;
		int clientCountNow = 0, clientCountOld = 0; 
    	int hasClient = 0;
		int videoCount = 0;
		int audioCount = 0;
		
		printf("wait client..........\n");
        rtsp[0]->V_Serial[0] = 0;
        rtsp[0]->V_Serial[1] = 0;
        rtsp[0]->A_Serial[0] = 0;
        rtsp[0]->A_Serial[1] = 0;

		if(AbortSignal == 1)
			break;
			
		printf("send begin..........\n");
	
    	int	get_audio_flag = 0;
    	
		printf("found I Frame sucess........\n");

    	char *ptr = NULL;
		int timestamp = 0;
        unsigned int stamp = 0;
		struct timeval _timestamp;
	    struct timezone tz;
		long old_time;
		int send_data_ret = 0;
    	while(AbortSignal == 0)
    	{
    		QBuff * pbuf = (QBuff *)OSAL_Dequeue(&pMs->mQueueBuffer);
			if (pbuf == NULL)
			{
				//printf("%s dequeue no buffer, sleep...\n",pMs->media_session_name);
				//usleep(1000000);
				usleep(1000);
				continue;
			}
			int j=0;
			unsigned char * tmp = (unsigned char *)pbuf->data;

			//printf("[%s %d] \r\n",__FUNCTION__,__LINE__);		

			if(pMs->conn_index == -1)
			{
				//pMs->conn_index = 0;
				usleep(50);
				continue;			
			}
	
			if(rtsp[pMs->conn_index]->is_sending_data == 1)
			{	
				if(rtsp[pMs->conn_index]->rtp_tcp_mode==RTP_TCP)
				{
					if(rtsp[pMs->conn_index]->rtsp_video_stream == 1)
					{					
						send_data_ret = build_rtp_tcp_nalu(pbuf->data,pbuf->size , pMs->conn_index,0,audio_encode_type,0,(pbuf->time/1000)*TIME_STAMP_S);//(pbuf->time*TIME_STAMP_S)&0x000000003fffffffll);
					}					                      
				}
				else
				{
					if(rtsp[pMs->conn_index]->rtsp_video_stream == 1)
					{
						build_rtp_nalu(pbuf->data ,pbuf->size , pMs->conn_index,0,audio_encode_type,0,(pbuf->time/1000)*TIME_STAMP_S);//(pbuf->time*TIME_STAMP_S)&0x000000003fffffffll);
					}				
				} 
			}  
					
    	}
        printf("%s----->%d  \n", __FUNCTION__, __LINE__);
    }
	//return 0;
}


/******************************************************************************/
/*
 *  send rtp	packet
 *	  cur_conn_num :	current connect number
 */
/******************************************************************************/
S32 rtp_send_packet(S32 cur_conn_num)
{
	//rtp_send_form_file(cur_conn_num);
	rtp_send_form_stream(cur_conn_num);
	return 0;
}

 

