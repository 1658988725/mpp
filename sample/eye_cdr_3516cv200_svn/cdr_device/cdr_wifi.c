#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/wait.h>
#include "cdr_wifi.h"
int cdr_wifi_init(void)
{
	cdr_init_wifi_config();
	cdr_system("/home/wifi_bin/init_wifi_7601.sh");	
	return 0;
}
