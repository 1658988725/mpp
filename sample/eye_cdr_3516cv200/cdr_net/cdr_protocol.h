#ifndef __CDR_PROTOCOL_H_
#define __CDR_PROTOCOL_H_

#define FIX_HEADER_C "EYEC"
#define FIX_HEADER_S "EYES"
#define FIE_HEADER_LEN 4
#define PROTOCOL_VERSION 0x01
#define CMD_EXTDATA_MAXLEN 256
#define CMD_SOCK_LEN_MIN 136

#define CDR_RETURN_LOGINERROR 		"LOGINERROR"
#define CDR_RETURN_LOGINLIMIT 		"LOGINLIMIT"
#define CDR_RETURN_OK 				"OK"
#define CDR_RETURN_AUTHERROR 		"AUTHERROR"
#define CDR_RETURN_PARAMERROR 		"PARAMERROR"
#define CDR_RETURN_PARAMVALUEERROR 	"PARAMVALUEERROR"
#define CDR_RETURN_ERROR 			"ERROR"
#define CDR_RETURN_FILENOTEXIST 	"FILENOTEXIST"
#define CDR_RETURN_NOTSD			"NOTSD"
#define CDR_RETURN_CRCERROR			"CRCERROR"


enum cdr_appprotocol_cmd
{
	CDR_login				=0x01,
	CDR_keep_live			=0x02,
	CDR_pic_index_list		=0x03,
	CDR_get_mp4_list		=0x04,
	CDR_stop_rec			=0x05,
	CDR_get_tmvideo 		=0x06,
	CDR_pic_capture 		=0x07,
	CDR_get_capture_list 	=0x08,
	CDR_get_sd_info 		=0x09,
	CDR_del_file			=0x0A,
	CDR_format_sd 			=0x0B,
	CDR_set_rec_time	 	=0x0C,
	CDR_get_system_cfg		=0x0D,
	CDR_set_system_cfg		=0x0E,
	CDR_system_settime		=0x0F,
	CDR_system_reset		=0x10,
	CDR_sofeware_update 	=0x11,
	CDR_get_gps_list		=0x12,
	
	CDR_get_smartunit_ID	=0xFD,
	CDR_device_discovery	=0xFF	
};


#define PACKED __attribute__( ( packed, aligned(1) ) )

#define MSGBODY_MAX_LEN 200
typedef struct cdr_protocol_header
{
	unsigned char HeadFlag[4];
	unsigned char CmdID;
	unsigned short MsgSN;
	unsigned char VersionFlag[2];
	unsigned char  Token[32];
	unsigned short	MsgLenth;
}PACKED CDR_PROTOCOL_HEADER;

typedef struct cdr_protocol
{
	CDR_PROTOCOL_HEADER proheader;
	char	MsgBody[MSGBODY_MAX_LEN];
	unsigned short VerifyCode;
}PACKED CDR_PROTOCOL;
#endif

