/*******
 * 
 * FILE INFO: rtspd version info 
 * project:	rtsp server
 * file:	rtsp/rtp/rtcp
 * started on:	2012/03/18 12:14:26
 * started by:	  
 * email:  
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:	2012/03/28 13:18:24
 * updated by:	
 * version: 1.2.0.0
 * 
 *******/

#include "version.h"

/******************************************************************************/
/*
 *	
 * Arguments: convert rtspd version info to str
 * sver: rtspd version info
 */
/******************************************************************************/
S32 convert_iver_2str(CHAR *sver)
{
	if (!sver)
		return -1;
	sprintf(sver, "%d.%d.%d.%d", 
		(RTSPD_VERSION>>24)&0xff, 
		(RTSPD_VERSION>>16)&0xff,
		(RTSPD_VERSION>>8)&0xff,
		(RTSPD_VERSION>>0)&0xff);
	return 0;
}

