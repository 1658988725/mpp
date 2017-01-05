#ifndef _RTP_UDP_H
#define _RTP_UDP_H
#include "../comm/type.h"

VOID *vd_rtp_func(VOID *arg);
VOID *vd_rtcp_recv(VOID *arg);
VOID *vd_rtcp_send(void * arg);
void *vd_rtcp_tcp_send(void *arg);




#endif

