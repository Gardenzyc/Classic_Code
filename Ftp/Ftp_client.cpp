#include <time.h>
#include "Ftp_client.h"
#if 0
enum {
	LSA_SIZEOF_SA = sizeof(
		union {
			struct sockaddr sa;
			struct sockaddr_in sin;
		}
	)
};
#endif

typedef struct ftpmissions{
	int currmission_id;
	int len_missionlist;
}ftpmissions_t;

typedef struct ftpbackupmission_info{
	int currfilelist_id;
	int len_filelist;
}ftpbackupmission_info_t;
 
int xatou(char *buf)
{
	char c;
	int i;
	int j = 0;
	int retval = 0;
	char mod[3] = {1, 10, 100};
	int len = strlen(buf);
	if ( (!len)||(len > 3) )
	{
		return -1;
	}	
 
	for (i = len - 1; i >= 0; i--)
	{
		c = buf[i];
		retval += atoi(&c)*mod[j++];
	}
	return retval;
}
int xatoul_range(char *buf, int low, int top)
{
	int retval = xatou(buf);
	if (retval < low)
	{
		retval = low;
	}	
 
	if (retval > top)
	{
		retval = top;
	}
//	printf("buf = %s\n", buf);	
//	printf("###retval = %d\n", retval);
	return retval;
}
len_and_sockaddr *xhost2sockaddr(const char *ip_addr, int port)
{
	int rc;
	len_and_sockaddr *r = NULL;
	struct addrinfo *result = NULL;
	struct addrinfo hint;
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	rc = getaddrinfo(ip_addr, NULL, &hint, &result);
	if (rc||!result)
	{
		free(r);		
		r = NULL;
		return r;
	}
	r = (len_and_sockaddr *)malloc(4 + result->ai_addrlen);
	if (r == NULL)
	{
		return NULL;
	}
	r->len = result->ai_addrlen;
	memcpy(&r->sa, result->ai_addr, result->ai_addrlen);
	r->sin.sin_port = htons(port);
	freeaddrinfo(result);	
	return r;	
}
/******************************************************************/
#define IGNORE_PORT NI_NUMERICSERV
char* sockaddr2str(const struct sockaddr *sa, int flags)
{
	char host[128];
	char serv[16];
	int rc;
	socklen_t salen;
 
	//salen = LSA_SIZEOF_SA;
	salen = sizeof(sockaddr);
	rc = getnameinfo(sa, salen,
			host, sizeof(host),
			/* can do ((flags & IGNORE_PORT) ? NULL : serv) but why bother? */
			serv, sizeof(serv),
			/* do not resolve port# into service _name_ */
			flags | NI_NUMERICSERV
			);
	if (rc)
		return NULL;
	if (flags & IGNORE_PORT)
		return strdup(host);
	/* For now we don't support anything else, so it has to be INET */
	/*if (sa->sa_family == AF_INET)*/
	char *retmsg = (char *)malloc(2048);
	memset(retmsg, 0, 2048);
	sprintf(retmsg, "%s:%s", host, serv);
	return retmsg;
	/*return xstrdup(host);*/
}
 
char* xmalloc_sockaddr2dotted(const struct sockaddr *sa)
{
	return sockaddr2str(sa, NI_NUMERICHOST);
}
 
int xopen3(const char *pathname, int flags, int mode)
{
	int ret;
 
	ret = open(pathname, flags, mode);
	if (ret < 0) {
		bb_perror_msg_and_die("can't open '%s'", pathname);
	}
	return ret;
}
int xopen(const char *pathname, int flags)
{
	return xopen3(pathname, flags, 0666);
}
 
void safe_fclose(FILE *control_stream)
{
	if (control_stream != NULL)
	{
		printf("+++++++++close ftp control_stream!!!!\n\n");
		fclose(control_stream);
		control_stream = NULL;
	}
}
 
ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;
 
	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);
 
	return n;
}
ssize_t safe_write(int fd, const void *buf, size_t count)
{
	ssize_t n;
 
	do {
		n = write(fd, buf, count);
	} while (n < 0 && errno == EINTR);
 
	return n;
}
size_t full_write(int fd, const void *buf, size_t len)
{
	ssize_t cc;
	ssize_t total;
 
	total = 0;
 
	while (len) {
		cc = safe_write(fd, buf, len);
 
		if (cc < 0)
			return cc;	/* write() returns -1 on failure. */
 
		total += cc;
		buf = ((const char *)buf) + cc;
		len -= cc;
	}
 
	return total;
}
 
 
 
off_t bb_full_fd_action(int src_fd, int dst_fd, off_t size)
{
	int status = -1;
	off_t total = 0;
	char buffer[BUFSIZ];
	if (src_fd < 0)
		goto out;
 
	if (!size) {
		size = BUFSIZ;
		status = 1; /* copy until eof */
	}
 
	while (1) {
		ssize_t rd;
		rd = safe_read(src_fd, buffer, size > BUFSIZ ? BUFSIZ : size);
 
		if (!rd) { /* eof - all done */
			status = 0;
			break;
		}
		if (rd < 0) {
			bb_perror_msg(bb_msg_read_error);
			break;
		}
		/* dst_fd == -1 is a fake, else... */
		if (dst_fd >= 0) {
			ssize_t wr = full_write(dst_fd, buffer, rd);
			if (wr < rd) {
				bb_perror_msg(bb_msg_write_error);
				break;
			}
		}
		total += rd;
		if (status < 0) { /* if we aren't copying till EOF... */
			size -= rd;
			if (!size) {
				/* 'size' bytes copied - all done */
				status = 0;
				break;
			}
		}
	}
out:
	return status ? -1 : total;
}
 
off_t bb_copyfd_eof(int fd1, int fd2)
{
	return bb_full_fd_action(fd1, fd2, 0);
}
/******************************************************************/
void ftp_die(const char *msg, const char *remote)
{
	/* Guard against garbage from remote server */
	const char *cp = remote;
	while (*cp >= ' ' && *cp < '\x7f') cp++;
	bb_error_msg_and_die("unexpected server response%s%s: %.*s\n",
			msg ? " to " : "", msg ? msg : "",
			(int)(cp - remote), remote);
//	assert(0);
}
 
int ftpcmd(const char *s1, const char *s2, FILE *stream, char *buf)
{
	unsigned n;
	if (s1) {
		if (s2) {
			fprintf(stream, "%s %s\r\n", s1, s2);
		} else {
			fprintf(stream, "%s\r\n", s1);
		}
	}
	do {
		char *buf_ptr;
 
		if (fgets(buf, 510, stream) == NULL) {
			bb_perror_msg_and_die("fgets");
			return -1;
		}
		buf_ptr = strstr(buf, "\r\n");
		if (buf_ptr) {
			*buf_ptr = '\0';
		}
	} while (!isdigit(buf[0]) || buf[3] != ' ');
 
	buf[3] = '\0';
	n = xatou(buf);
	buf[3] = ' ';
	return n;
}
 
void set_nport(len_and_sockaddr *lsa, unsigned port)
{
	if (lsa->sa.sa_family == AF_INET) {
		lsa->sin.sin_port = port;
		return;
	}
	/* What? UNIX socket? IPX?? :) */
}

// Die with an error message if we can't open a new socket.
int xsocket(int domain, int type, int protocol)
{
	int r = socket(domain, type, protocol);
 
	if (r < 0) {
		/* Hijack vaguely related config option */
#if ENABLE_VERBOSE_RESOLUTION_ERRORS
		const char *s = "INET";
		if (domain == AF_PACKET) s = "PACKET";
		if (domain == AF_NETLINK) s = "NETLINK";
		USE_FEATURE_IPV6(if (domain == AF_INET6) s = "INET6";)
			bb_perror_msg_and_die("socket(AF_%s)", s);
#else
		bb_perror_msg_and_die("socket");
#endif
	}
	return r;
}
 
int xclose(int s)
{
	if (s > 0)
	{
		close(s);
		s = -1;
	}
}

void xconnect(int s, const struct sockaddr *s_addr, socklen_t addrlen)
{
 
	if (connect(s, s_addr, addrlen) < 0) {
		if (s_addr->sa_family == AF_INET)
			bb_perror_msg_and_die("%s (%s)\n",
					"cannot connect to remote host",
					inet_ntoa(((struct sockaddr_in *)s_addr)->sin_addr));
		bb_perror_msg_and_die("cannot connect to remote host\n");
//		assert(0);
	}
}

int xconnect_stream(const len_and_sockaddr *lsa)
{
	int fd = xsocket(lsa->sa.sa_family, SOCK_STREAM, 0);
	xconnect(fd, &lsa->sa, lsa->len);
	return fd;
}

int xconnect_ftpdata(ftp_host_info_t *server, char *buf)
{
	char *buf_ptr;
	unsigned short port_num;
 
	//printf("buf = %s\n", buf);
	/* Response is "NNN garbageN1,N2,N3,N4,P1,P2[)garbage]
	 * Server's IP is N1.N2.N3.N4 (we ignore it)
	 * Server's port for data connection is P1*256+P2 */
	buf_ptr = strrchr(buf, ')');
	if (buf_ptr) *buf_ptr = '\0';
 
	buf_ptr = strrchr(buf, ',');
	*buf_ptr = '\0';
	port_num = xatoul_range(buf_ptr + 1, 0, 255);
 
	buf_ptr = strrchr(buf, ',');
	*buf_ptr = '\0';
	port_num += xatoul_range(buf_ptr + 1, 0, 255) * 256;
	
	//printf("#### port_num = %d\n", port_num);
	set_nport(server->lsa, htons(port_num));
	return xconnect_stream(server->lsa);
}
 
FILE *ftp_login(ftp_host_info_t *server)
{
	FILE *control_stream;
	char buf[512];
	int login_fd;
	/* Connect to the command socket */
	login_fd = xconnect_stream(server->lsa);
	if(login_fd == -1)
		return NULL;
	
	control_stream = fdopen(login_fd, "r+");
	if (control_stream == NULL) {
		xclose(login_fd);
		/* fdopen failed - extremely unlikely */
		bb_perror_nomsg_and_die();
		return NULL;
	}
	if (ftpcmd(NULL, NULL, control_stream, buf) != 220) {
		ftp_die(NULL, buf);
		safe_fclose(control_stream);
		return NULL;
	}
	/*  Login to the server */
	switch (ftpcmd("USER", server->user, control_stream, buf)) {
		case 230:
			break;
		case 331:
			if (ftpcmd("PASS", server->password, control_stream, buf) != 230) {
				ftp_die("PASS", buf);
				safe_fclose(control_stream);
				return NULL;
			}
			break;
		default:
			{
				ftp_die("USER", buf);
				safe_fclose(control_stream);
				return NULL;
			}
	}
	if(ftpcmd("TYPE I", NULL, control_stream, buf) == -1)
	{
		safe_fclose(control_stream);
		return NULL;
	}
	return control_stream;
}
 
int ftp_mkdir(FILE *control_stream, const char *FoldName)
{
	if (strlen(FoldName) <= 0)
	{
		return -1;
	}
	char buf[512];
	int response = ftpcmd("MKD", FoldName, control_stream, buf);
	if(response != 257)
	{
		//printf("MKD1111111111111111111111:%d\n", response);
		ftp_die("MKD", buf);
		return -1;	
	}
	//printf("MKD2222222222222222222:%d\n", response);
	return 0;
}
 
int ftp_cd(FILE *control_stream, const char *FoldName)
{
	if (strlen(FoldName) <= 0)
	{
		return -1;
	}
	char buf[512];
	int response = ftpcmd("CWD", FoldName, control_stream, buf);
	if(response != 250)
	{
		//printf("CWD1111111111111111111111response:%d\n", response);
		ftp_die("CWD", buf);
		return -1;	
	}
	//printf("CWD2222222222222222222response:%d\n", response);
	return 0;
}
 
int ftp_send(ftp_host_info_t *server, FILE *control_stream,
		const char *server_path, char *local_path)
{
	struct stat sbuf;
	char buf[512];
	int fd_data;
	int fd_local;
	int response;
 
	/*  Connect to the data socket */
	if (ftpcmd("PASV", NULL, control_stream, buf) != 227) {
		ftp_die("PASV", buf);
		return -1;
	}
	fd_data = xconnect_ftpdata(server, buf);
	if(fd_data == -1)
		return -1;
	/* get the local file */
	fd_local = STDIN_FILENO;
	if (NOT_LONE_DASH(local_path)) {
		fd_local = xopen(local_path, O_RDONLY);
		while(fd_local == -1)
		{
			xclose(fd_local);
			sleep(1);
			fd_local = xopen(local_path, O_RDONLY);
		}
		fstat(fd_local, &sbuf);
 
		sprintf(buf, "ALLO %"OFF_FMT"u", sbuf.st_size);
		response = ftpcmd(buf, NULL, control_stream, buf);
		switch (response) {
			case 200:
			case 202:
				break;
			default:
				{
					xclose(fd_data);
					xclose(fd_local);
					ftp_die("ALLO", buf);
					return -1;
				}
		}
	}
 
	response = ftpcmd("STOR", server_path, control_stream, buf);
	switch (response) {
		case 125:
		case 150:
			break;
		default:
			{
				xclose(fd_data);
				xclose(fd_local);
				ftp_die("STOR", buf);
				return -1;
			}
	}
 
		
	/* transfer the file  */
	ftpmissions_t missionlist={0,0};
	ftpbackupmission_info_t filelist={0,0};
	do{
		do{
			if (bb_copyfd_eof(fd_local, fd_data) == -1) {
				/* close it all down */
				xclose(fd_data);
				xclose(fd_local);
				return -1;
			//exit(EXIT_FAILURE);
	 		 }
		}while(filelist.currfilelist_id++<=filelist.len_filelist);
	}while (missionlist.currmission_id++<=missionlist.len_missionlist);
 
	/* close it all down */
	close(fd_data);
	close(fd_local);
	if (ftpcmd(NULL, NULL, control_stream, buf) != 226) {
		ftp_die("close", buf);
		return -1;
	}
	return EXIT_SUCCESS;
}
int ftp_quit(FILE *control_stream)
{
	if (control_stream == NULL)
	{
		return -1;
	}
	char buf[512];
	ftpcmd("QUIT", NULL, control_stream, buf);
	safe_fclose(control_stream);
	return 0;
}

Client_Param_t ClientParam = {
	.UserName = "zyc_ftp",
	.Passwd = "123456",
	.ServerIp = "192.168.56.101",
	.Port = 21,
	.Session = NULL
};

int Ftp_LoginCmd(void)
{
	ftp_host_info_t *server;
	server = (ftp_host_info_t *)malloc(sizeof(*server));
	if (server == NULL)
	{
		return ERROR;
	}
	server->user = ClientParam.UserName;
	server->password = ClientParam.Passwd;
	server->lsa = xhost2sockaddr(ClientParam.ServerIp, 21);
	printf("Connecting to %s (%s)\n", ClientParam.ServerIp, \
			xmalloc_sockaddr2dotted(&server->lsa->sa));
	ClientParam.Session  = ftp_login(server);
	if(NULL == ClientParam.Session)
	{
		return ERROR;
	}
	return OK;
}

/* 涓婁紶鑷虫湇鍔″櫒鏂囦欢鍚嶏紝鏈湴鏂囦欢鍚?*/
int Ftp_SendCmd(const char *server_path, char *local_path)
{
	ftp_host_info_t *server = (ftp_host_info_t *)malloc(sizeof(ftp_host_info_t));
	server = (ftp_host_info_t *)malloc(sizeof(*server));
	server->user = ClientParam.UserName;
	server->password = ClientParam.Passwd;
	server->lsa = xhost2sockaddr(ClientParam.ServerIp, ClientParam.Port);
	
	if(NULL == ClientParam.Session)
	{
		return ERROR;
	}
	return ftp_send(server, ClientParam.Session, server_path, local_path);
}

int Ftp_MkdirCmd(char *DirName)
{
	if(NULL == ClientParam.Session || NULL == DirName)
	{
		return ERROR;
	}
	return ftp_mkdir(ClientParam.Session, DirName);
}

int Ftp_CdCmd(char *DirName)
{
	int Ret;
	if(NULL == ClientParam.Session || NULL == DirName)
	{
		return ERROR;
	}
	Ret = ftp_cd(ClientParam.Session, "RMU_MAXSUS_TEST");
	if (Ret < 0)
	{
		Ftp_MkdirCmd(DirName);
		Ret = ftp_cd(ClientParam.Session, DirName);
	}
	return Ret;
}

void Ftp_QuitCmd(void)
{
	ftp_quit(ClientParam.Session);
}

void Ftp_MdfProc(void)
{
	
}

/* ServerNm: server file name; LocalLink: local path + file name */
int Ftp_UploadFile(char *ServerNm, char *LocalLink)
{
	if(NULL == ServerNm || NULL == LocalLink)
	{
		return ERROR;
	}
	if(ERROR == Ftp_SendCmd( ServerNm, LocalLink))
	{
		Ftp_QuitCmd();
		printf("\nConnecting to %s (%d)\n", ClientParam.ServerIp, ClientParam.Port);
		while(ClientParam.Session == NULL)
		{
			printf("reconnect...\n");
			Ftp_LoginCmd();
		}
		return ERROR;
	}

	return OK;
}

int Ftp_SearchFileUpload(char *DirPath, char *SearchCon)
{
	DIR *DirFd;
	struct dirent *DretFd;
	char LocalPath[128];
	char ServerPath[128];
	
	if(NULL == DirPath || NULL == SearchCon)
	{
		return ERROR;
	}

	DirFd = opendir(DirPath);
	if(NULL == DirFd)
	{
		printf("open dir[%s] error\n", DirPath);
		return ERROR;
	}

	while(NULL != (DretFd = readdir(DirFd)))
	{
		if(strstr(DretFd->d_name, SearchCon))
		{
			printf("Search filename : %s\n", DretFd->d_name);
			snprintf(LocalPath, sizeof(LocalPath), "%s/%s", DirPath, DretFd->d_name);
			if(ERROR == Ftp_SendCmd( DretFd->d_name, LocalPath))
			{
				Ftp_QuitCmd();
				printf("\nConnecting to %s (%d)\n", ClientParam.ServerIp, ClientParam.Port);
				while(ClientParam.Session == NULL)
				{
					printf("reconnect...\n");
					Ftp_LoginCmd();
				}
			}
		}
	}

	closedir(DirFd);
	return OK;
}

#if 0
struct tm
{
    int tm_sec;  /*绉掞紝姝ｅ父鑼冨洿0-59锛?浣嗗厑璁歌嚦61*/
    int tm_min;  /*鍒嗛挓锛?-59*/
    int tm_hour; /*灏忔椂锛?0-23*/
    int tm_mday; /*鏃ワ紝鍗充竴涓湀涓殑绗嚑澶╋紝1-31*/
    int tm_mon;  /*鏈堬紝 浠庝竴鏈堢畻璧凤紝0-11*/  1+p->tm_mon;
    int tm_year;  /*骞达紝 浠?900鑷充粖宸茬粡澶氬皯骞?/  1900锛?p->tm_year;
    int tm_wday; /*鏄熸湡锛屼竴鍛ㄤ腑鐨勭鍑犲ぉ锛?浠庢槦鏈熸棩绠楄捣锛?-6*/
    int tm_yday; /*浠庝粖骞?鏈?鏃ュ埌鐩墠鐨勫ぉ鏁帮紝鑼冨洿0-365*/
    int tm_isdst; /*鏃ュ厜鑺傜害鏃堕棿鐨勬棗鏍*/
};

rec_1053_2019-05-05-12_44_33.dat
#endif

int GetSystemTime(SystemTime_t *SysTime)
{
	if(NULL == SysTime)
	{
		return ERROR;
	}

	const time_t TimeTem = time(NULL);
	printf("time_t = %ld\n", TimeTem);
	struct tm *TmTem = localtime(&TimeTem);
	printf("current tm : %s\n", asctime(TmTem));

	SysTime->Year = 1900+TmTem->tm_year;
	SysTime->Month = 1+TmTem->tm_mon;
	SysTime->Day = TmTem->tm_mday;
	SysTime->Hour = TmTem->tm_hour;
	SysTime->Min = TmTem->tm_min;
	SysTime->Sec = TmTem->tm_sec;

	return OK;
}

int Ftp_FileCreate(char *CarVin, char *Path, char *FileName)
{
	char PathNm[FILE_PATH_LEN] = {0};
	int FileFd;
	SystemTime_t SysTime;
	
	if(NULL == CarVin || NULL == Path)
	{
		return ERROR;
	}

	GetSystemTime(&SysTime);
	snprintf(FileName, FILE_NAME_LEN, "rec_%s_%d-%02d-%02d-%02d_%02d_%02d.dat", CarVin, SysTime.Year, \
		SysTime.Month, SysTime.Day, SysTime.Hour, SysTime.Min, SysTime.Sec);
	snprintf(PathNm, FILE_PATH_LEN, "%s/rec_%s_%d-%02d-%02d-%02d_%02d_%02d.dat", Path, CarVin, SysTime.Year, \
		SysTime.Month, SysTime.Day, SysTime.Hour, SysTime.Min, SysTime.Sec);

	FileFd = open(PathNm, O_RDWR|O_CREAT|O_APPEND);
	
	return FileFd;
	
}

int Ftp_FileClose(int Fd)
{
	if(Fd < 0)
	{
		return ERROR;
	}

	return close(Fd);
}

int Ftp_FileWrite(int Fd, const void *Buff, int Count)
{
	if(Fd < 0 || NULL == Buff)
	{
		return ERROR;
	}

	return write(Fd, Buff, Count);
}

int Ftp_FileRead(int Fd, void *Buff, int Count)
{
	if(Fd < 0 || NULL == Buff)
	{
		return ERROR;
	}

	return read(Fd, Buff, Count);
}

static inline void GetWaitTime(int WaitMs, struct timespec *Tp)
{
	time_t sec, t;
	long long nsec;

	sec = 0;
	if (WaitMs == NO_WAIT)
	{
		nsec = 0;
	}
	else
	{
		nsec = WaitMs * 1000000LL;
	}

	if (clock_gettime(CLOCK_REALTIME, Tp) == -1)
	{
		printf("getTimespec: clock_gettime call fail, error %d(%s)\n", errno, strerror(errno));
		Tp->tv_sec = time(NULL) + 1;
		Tp->tv_nsec = 0;
	}
	else
	{
		t = time(NULL) + 1;
		if ((int)(Tp->tv_sec - t) > 30) 
		{
			Tp->tv_sec = t;
			Tp->tv_nsec = 0;
		}
	}

	nsec += Tp->tv_nsec;
	printf("getTimespec: current time sec = %ld, time = %ld, nsec = %ld, total nsec = %lld\n", 
			Tp->tv_sec, time(NULL)+1, Tp->tv_nsec, nsec);
	if (nsec >= 1000000000)
	{
		sec = nsec / 1000000000;
		nsec = nsec % 1000000000;
	}
	Tp->tv_sec += sec;
	Tp->tv_nsec = nsec;
	printf("GetWaitTime: after time sec = %ld, time = %ld, nsec = %ld\n", 
			Tp->tv_sec, time(NULL)+1, Tp->tv_nsec);

	return;
}

static mqd_t s_FtpMq = -1;

int Ftp_MqInit(void)
{
	struct mq_attr FtpMqAttr;
	FtpMqAttr.mq_maxmsg = FTP_MQ_MAX_NUM;
	FtpMqAttr.mq_msgsize = sizeof(FtpMqInfo_t);
	mq_unlink(FTP_MQ_NAME);
	
	s_FtpMq = mq_open(FTP_MQ_NAME, O_CREAT|O_EXCL|O_RDWR, 0600, &FtpMqAttr);
	if(-1 == s_FtpMq)
	{
		printf("ftpmq create error!\n");
		return ERROR;
	}

	return OK;
}

int Ftp_MqSend(const char *pMsg, size_t MsgLen, int WaitMs)
{
	uint8_t iRet = ERROR;
	struct timespec TimeOut;
	
	if(NULL == pMsg || 0 == MsgLen || -1 == s_FtpMq)
	{
		return ERROR;
	}
	
	if(WAIT_FOREVER == WaitMs)
	{
		iRet = mq_send(s_FtpMq, pMsg, MsgLen, 0);
	}
	else
	{
		GetWaitTime(WaitMs, &TimeOut);
		iRet = mq_timedsend(s_FtpMq, pMsg, MsgLen, 0, &TimeOut);
	}
	return iRet;
}

int Ftp_MqRecv(char *pMsg, size_t MsgLen, int WaitMs)
{
	uint8_t iRet = ERROR;
	struct timespec TimeOut;
	
	if(NULL == pMsg || 0 == MsgLen || -1 == s_FtpMq)
	{
		return ERROR;
	}

	if(WAIT_FOREVER == WaitMs)
	{
		iRet = mq_receive(s_FtpMq, pMsg, MsgLen, 0);
	}
	else
	{
		GetWaitTime(WaitMs, &TimeOut);
		iRet = mq_timedreceive(s_FtpMq, pMsg, MsgLen, 0, &TimeOut);
	}
	return iRet;
}

/* test mode */
void* Ftp_MqSendTask(void *param)
{
	FtpMqInfo_t Frame;
	int MsgLen;
	int i;
	for(i=0; i<1000; i++)
	{
		Frame.CanId = i;
		//Frame.Channel = i;
		
		MsgLen = Ftp_MqSend((char*)&Frame, sizeof(FtpMqInfo_t), 10);
		if(MsgLen>0)
		{
			printf("Send Ftp Mq success!!!\n");
		}
		usleep(100);
	}
}

void* Ftp_MqRecvTask(void *param)
{
	int MsgLen = -1;
	int FtpFd = -1;
	int nFileSize = -1;
	char FileName[FILE_NAME_LEN] = {0};
	char LocalLink[FILE_PATH_LEN] = {0};
	FtpMqInfo_t FtpMqMsg;
	memset(&FtpMqMsg, 0, sizeof(FtpMqInfo_t));

	int ConStatus = Ftp_LoginCmd();
	Ftp_CdCmd("RMU_MAXSUS");

	if (ConStatus == ERROR)
	{
		printf("login failed!\n");
		//return ERROR;
	}
	
	do{
		MsgLen = Ftp_MqRecv((char*)&FtpMqMsg, sizeof(FtpMqInfo_t), 10);
		if(sizeof(FtpMqInfo_t) == MsgLen)
		{
			printf("FTP RECV : %d\n", FtpMqMsg.CanId);
			/* write file */
			if(-1 == FtpFd)
			{
				FtpFd = Ftp_FileCreate(FTP_VIN_CODE, FTP_LOCAL_SAVE_PATH, FileName);
				printf("create File\n");	
			}
			else
			{
        			nFileSize = lseek(FtpFd, 0L, SEEK_END);
				if(nFileSize <= PER_FILE_SIZE)
				{
					Ftp_FileWrite(FtpFd, &FtpMqMsg, MsgLen);
				}
				else
				{
					Ftp_FileWrite(FtpFd, &FtpMqMsg, MsgLen);
					Ftp_FileClose(FtpFd);
					snprintf(LocalLink, FILE_PATH_LEN, "%s/%s", FTP_LOCAL_SAVE_PATH, FileName);
					Ftp_UploadFile(FileName, LocalLink);
					FtpFd = -1;
				}
			}
			
		}
	}while(1);
}

#define TEST 0
#if TEST
int main(void)
{
	//const char *port = "ftp";
	int ConStatus = ERROR;
	char local_path[64];
	char server_path[64];
	
	ConStatus = Ftp_LoginCmd();
	
	if (ConStatus == ERROR)
	{
		printf("login failed!\n");
		return ERROR;
	}

	Ftp_FileCreate(FTP_VIN_CODE, FTP_LOCAL_SAVE_PATH);
		
	Ftp_CdCmd("RMU_MAXSUS_TEST");
	
	Ftp_SearchFileUpload(FTP_LOCAL_SAVE_PATH, "05-13");
	printf("%s upload success\n",local_path);
	Ftp_QuitCmd();
	
	return OK;
}

#endif
