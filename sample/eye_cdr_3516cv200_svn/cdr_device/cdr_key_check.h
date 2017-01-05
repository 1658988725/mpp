
#ifndef KEY_H
#define KEY_H

typedef int (*cdr_key_check_callback)(unsigned int uiRegValue);

int cdr_init_key_check();

void cdr_key_check_setevent_callbak(cdr_key_check_callback callback);

#endif



