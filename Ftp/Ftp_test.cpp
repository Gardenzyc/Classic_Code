#include "Ftp_client.h"

#define FTP_SERVER_FILE "rec_zyc_2019-06-24-10_51_32.dat"
#define FTP_DOWN_FILE "./RMU_MAXSUS_TEST/rec_zyc_2019-06-24-10_51_32.dat"

#define FTP_LOCAL_FILE "./RMU_MAXSUS_TEST/rec_zyc_2019-07-15-11_10_20.dat"

int main(void)
{
	//const char *port = "ftp";
	int ConStatus = ERROR;
	int Fd;
	char Buff[] = "I can get it! Just keep!";
	char Data[64] = {0};
	char FileNm[FILE_NAME_LEN] = {0};

	Client_Param_t ClientCurParam = {
	.UserName = "zyc_ftp",
	.Passwd = "123456",
	.ServerIp = "10.64.209.209",
	.Port = 21,
	.Session = NULL
	};
	Ftp_SetLoginServInfo(&ClientCurParam);
	
	ConStatus = Ftp_LoginCmd();
	
	if (ConStatus == ERROR)
	{
		printf("login failed!\n");
		return ERROR;
	}

	Fd = Ftp_FileCreate(FTP_VIN_CODE, FTP_LOCAL_SAVE_PATH, FileNm);
	printf("FILE NAME : %s\n", FileNm);
	Ftp_FileWrite(Fd, Buff, sizeof(Buff));
	read(Fd, Data, sizeof(Data));
	printf("read File Data: %s, %s\n", Data, Buff);
	Ftp_FileClose(Fd);
	
	Ftp_CdCmd("RMU_MAXSUS_TEST");
	Ftp_UploadFile("rec_zyc_2019-07-15-11_10_20.dat", FTP_LOCAL_FILE);
	//Ftp_SendCmd("rec_zyc_2019-07-15-11_10_20.dat", FTP_LOCAL_FILE, CMD_UPLOAD_FILE);
//	Ftp_SearchFileUpload(FTP_LOCAL_SAVE_PATH, "05-14");
	printf("%s upload success\n",FTP_LOCAL_SAVE_PATH);
	//Ftp_QuitCmd();
	#if 0
	Ftp_MqInit();

	pthread_t FtpRecvId;
	pthread_create(&FtpRecvId, NULL, Ftp_MqRecvTask, NULL);

	pthread_t FtpSendId;
	pthread_create(&FtpSendId, NULL, Ftp_MqSendTask, NULL);
	pthread_join(FtpSendId, NULL);	
	#endif
	
	Ftp_SendCmd(FTP_SERVER_FILE, FTP_DOWN_FILE, CMD_DOWNLOAD_FILE);
	return OK;
}
