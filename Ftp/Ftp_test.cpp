#include "Ftp_client.h"

int main(void)
{
	//const char *port = "ftp";
	int ConStatus = ERROR;
	int Fd;
	char Buff[] = "I can get it! Just keep!";
	char Data[64] = {0};
	char FileNm[FILE_NAME_LEN] = {0};
	
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
	
//	Ftp_SearchFileUpload(FTP_LOCAL_SAVE_PATH, "05-14");
	printf("%s upload success\n",FTP_LOCAL_SAVE_PATH);
	Ftp_QuitCmd();
	
	Ftp_MqInit();

	pthread_t FtpRecvId;
	pthread_create(&FtpRecvId, NULL, Ftp_MqRecvTask, NULL);

	pthread_t FtpSendId;
	pthread_create(&FtpSendId, NULL, Ftp_MqSendTask, NULL);
	pthread_join(FtpSendId, NULL);	
	return OK;
}
