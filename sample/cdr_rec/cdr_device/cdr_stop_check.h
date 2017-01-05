
#ifndef STOP_APP_H
#define STOP_APP_H

typedef int (*cdr_stop_check_callback)(unsigned int uiRegValue);

int cdr_init_stop_check();

void cdr_stop_check_setevent_callbak(cdr_stop_check_callback callback);

#endif



