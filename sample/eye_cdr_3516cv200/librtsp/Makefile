####ROOTDIR = ../../..
#####include $(ROOTDIR)/Rules.make
####CFLAGS +=  -I$(PUBLIC_INCLUDE_DIR)  -I../boa-0.94.13/src   
LDFLAGS += -g -fPIC -O2 -rdynamic -ldl
TARGETS = rtspd
OBJS = comm/version.o  comm/parese_conf.o comm/md5c.o rtsp/rtsp_comm_fun.o rtsp/rtsp_options.o rtsp/rtsp_describe.o \
	rtsp/rtsp_setup.o rtsp/rtsp_play.o rtsp/rtsp_teardown.o rtsp/rtsp_status.o \
	rtp/rtp_h264.o rtcp/rtcp.o  rtp/udp.o rtp/tcp.o net/socket.o rtspd.o  

	###../boa-0.94.13/src/net_config.o 
INCPATH := -I utils/

LIBS = -lpthread
####EXTRALIBS :=  $(APP_LIB_DIR)/Appro_interface.a  $(APP_LIB_DIR)/msg_util.a $(CMEM_INSTALL_DIR)/packages/ti/sdo/linuxutils/cmem/lib/cmem.a470MV  $(APP_LIB_DIR)/file_msg_drv.a  $(APP_LIB_DIR)/sysctrl.a $(APP_LIB_DIR)/sys_msg_drv.a $(APP_LIB_DIR)/alarm_msg_drv.a
CC = $(MVTOOL_PREFIX)gcc
all:$(TARGETS)
		
.%o:%.c
	$(CC) $(CFLAGS) -c $^

install:
	cp -f $(TARGETS) $(EXEC_DIR)

tftp:
	cp -f $(TARGETS) $(TFTP_HOME)
	
$(TARGETS):$(OBJS) 
	##$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(EXTRALIBS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(INCPATH)
#$(CC) $(LIBS) -o $@ $^ $(LDFLAGS) #ERROR 
	$(MVTOOL_PREFIX)strip $(TARGETS)
	
clean:
	rm -f $(OBJS) *~ 
	rm -f $(TARGETS)
