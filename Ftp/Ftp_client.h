#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <errno.h>
#include <assert.h>
#include <netinet/in.h>
#include <dirent.h>
#include <ctype.h>
 #include <pthread.h>
 #include <mqueue.h>
 
using namespace std;
 
#define bb_perror_msg(x) printf(x)
#define bb_msg_read_error "Error: bb_msg_read_error"
#define bb_msg_write_error "Error: bb_msg_write_error"
#define NOT_LONE_DASH(s) ((s)[0] != '-' || (s)[1])
#define bb_perror_msg_and_die printf
#define bb_error_msg_and_die printf
#define bb_perror_nomsg_and_die() assert(0);
 
//#define TRACE() printf("%s %d\n", __FUNCTION__, __LINE__);
#define OFF_FMT "l"
#define ERROR -1
#define OK 0

#define FTP_LOCAL_SAVE_PATH "./RMU_MAXSUS_TEST"
#define FTP_VIN_CODE "zyc"

#define FTP_FILE_MQ "/ftp_file"
#define FILE_NAME_LEN 128
#define FILE_PATH_LEN 256

typedef struct{
	const char *UserName;
	const char *Passwd;
	const char *ServerIp;
	short Port;
	FILE *Session;
}Client_Param_t;

typedef struct len_and_sockaddr {
	socklen_t len;
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
	};
} len_and_sockaddr;
 
typedef struct ftp_host_info_s {
	const char *user;
	const char *password;
	struct len_and_sockaddr *lsa;
} ftp_host_info_t;

typedef struct{
	int Year;
	int Month;
	int Day;
	int Hour;
	int Min;
	int Sec;
}SystemTime_t;

#define FTP_MQ_NAME "/ftp_mq"
#define FTP_MQ_MAX_NUM 10
#define WAIT_FOREVER -1
#define NO_WAIT 0	
#define PER_FILE_SIZE 10000
#define FRAME_SIZE 64

typedef struct{
	uint8_t Channel;
	uint32_t CanId;
	uint8_t Data[FRAME_SIZE];
}FtpMqInfo_t;

extern Client_Param_t ClientParam;

#ifdef  __cplusplus
extern "C" 
{
#endif
	/* FTP 鐧诲綍淇℃伅*/
	//Client_Param_t ClientParam;
	/* 鐧诲綍FTP鏈嶅姟鍣?*/
	int Ftp_LoginCmd(void);
	/* 涓婁紶鑷虫湇鍔″櫒鏂囦欢鍚嶏紝鏈湴鏂囦欢鍚?*/
	int Ftp_SendCmd(const char *server_path, char *local_path);
	/* 鍒涘缓FTP 鏈嶅姟鍣ㄧ洰褰?*/
	int Ftp_MkdirCmd(char *DirName);
	/* 鍒囨崲杩滅▼鏈嶅姟鍣ㄧ洰褰?*/
	int Ftp_CdCmd(char *DirName);
	/* 鍏抽棴FTP瀹㈡埛绔湇鍔″櫒杩炴帴*/
	void Ftp_QuitCmd(void);
	/* 鎼滅储璁惧FTP 鏂囦欢澶瑰搴旀枃浠讹紝骞惰繘琛屼笂浼?*/
	int Ftp_SearchFileUpload(char *DirPath, char *SearchCon);
	/* 鍒涘缓鏈湴FTP 鏂囦欢*/
	int Ftp_FileCreate(char *CarVin, char *Path, char *FileName);
	/* 鍏抽棴鎵撳紑鐨勬枃浠?*/
	int Ftp_FileClose(int Fd);
	/* 璇诲彇鏂囦欢鍐呭*/
	int Ftp_FileRead(int Fd, void *Buff, int Count);
	/* 鍐欏叆鏂囦欢鍐呭*/
	int Ftp_FileWrite(int Fd, const void *Buff, int Count);
	/* 鎺ユ敹澶勭悊绾跨▼*/
	int Ftp_MqInit(void);
	void* Ftp_MqRecvTask(void *param);
	void* Ftp_MqSendTask(void *param);
#ifdef __cplusplus
}
#endif
 
#endif
