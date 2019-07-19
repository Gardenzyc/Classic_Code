#include "Storage.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include "Storage.h"
#include <dirent.h>
#include "Dlist.h"
#include <assert.h>

#define RMU_FILE_SIZE 1024*512
#define RMU_MAX_PATH 256
#define CREAT_FILE_FLAGS (O_RDWR|O_CREAT|O_CLOEXEC)
#define RMU_FILE_HANDLE_INVALID -1

typedef struct{
	char Name[RMU_MAX_PATH];
	RMU_FILE_HANDLE FileHdl;
	long long Pos;
	Dlist_t List;	
}RmuFile_t; 

typedef struct{
	char Name[RMU_MAX_PATH];
	off_t Size;
	time_t FileTime;
	Dlist_t List;	
}RmuSerch_t; 

typedef enum{
	FILE_TYPE,
	PATH_TYPE,
	INEXISTENCE
}PATH_TYPE_T;

static Dlist_t s_FileManager;
static Dlist_t s_FileList;

static PATH_TYPE_T IsValidPath(const char *Path)
{
	unsigned Len = strlen(Path);
	PATH_TYPE_T Ret = INEXISTENCE;
	struct stat StatTem;
	
	if(!Path || !Len)
		return INEXISTENCE;

	if(!stat(Path, &StatTem))
		if(S_ISDIR(StatTem.st_mode))
			Ret = PATH_TYPE;//dir
		else
			Ret = FILE_TYPE;//file

	return Ret;
}

static int GetFileName(char *Name, int NameLen, char *Path)
{
	char *pNm = strrchr(Path, '/');//last '/' position 
	int NameLenTem = strlen(pNm);

	if(!pNm)
		pNm = Path;
		
	if(NULL == Name || !NameLen || NULL == Path)
		return false;
	
	if(NameLenTem > NameLen)
		return false;

	strcpy(Name, pNm);
	return true;
}

static int GetDirPath(char *Dir, int PathLen, char *Path)
{
	int PathLenTem = 0;
	char *pTem = NULL;
	if(NULL == Path || !PathLen || NULL == Path)
		return false;

	pTem  = strrchr(Path, '/');
	if(!pTem)
		return false;
	
	PathLenTem = pTem - Path;

	if(PathLenTem + 1 > PathLen)
	{
		printf("Path Len is over!\n");
		return false;
	}

	strncpy(Dir, Path, PathLenTem);
	Dir[PathLenTem] = '\0';//0
	return true;
}

static int Mkdir(const char *Path)
{
	if(NULL == Path)
		return false;

	if(mkdir(Path, S_IRWXU|S_IRWXG|S_IRWXO))//#define 
	{
		printf("mkdir path(%s) failed!\n ", Path);
		return false;
	}

	return true;
}

bool RmuSerchFileList(char *Path, Dlist_t *FileList)
{
	DIR *Dfd = NULL;
	struct dirent *Dp = NULL;
	struct stat StatTem;
	
	if(INEXISTENCE ==IsValidPath(Path))
	{
		if(!Mkdir(Path))
			return false;
	}
	else if(PATH_TYPE ==IsValidPath(Path))
	{
		if(!(Dfd = opendir(Path)))
		{
			printf("opendir [%s] error\n", Path);
			closedir(Dfd);
			return false;
		}
		else
		{
			while((Dp = readdir(Dfd)) != NULL)
			{
				if((!strcmp(Dp->d_name, ".")) ||(!strncmp(Dp->d_name, "..", 2)) )
					continue;

				if(!stat(Path, &StatTem))
				{
					if(S_ISDIR(StatTem.st_mode))
					{
						continue;
					}
					else
					{
						RmuSerch_t *pFileListInfo = malloc(sizeof(RmuSerch_t));//file
						strcpy(pFileListInfo->Name, Dp->d_name);
						pFileListInfo->FileTime = StatTem.st_mtime;
						pFileListInfo->Size = StatTem.st_size;
						DlistAddTail(&s_FileList, &pFileListInfo->List);
					}
				}
			}
		}
	}
	else
	{
		printf("[%s] is file name\n", Path);
		return false;
	}
	
}

static int _Rmu_StorInit(void)
{
	DlistInit(&s_FileManager);
	return true;
}

int _Rmu_StorDeInit(void)
{
	RmuFile_t *pFile;
	Dlist_t *pDlist = NULL;
	int Count = 0;
	
	while(pDlist = DlistQueryNext(&s_FileManager, pDlist))
	{
		pFile = GET_CONTAINER_OF(pDlist, RmuFile_t, List);
		Count++;
		printf("File Manager is not empty, %d:%s\n", Count, pFile->Name);
		//free(pFile);//release 
	}
}

RMU_FILE_HANDLE _Rmu_StorOpen(char *Name, unsigned Flag)
{
	char DirTem[RMU_MAX_PATH];
	RMU_FILE_HANDLE Fd;
	RmuFile_t *File;
	
	if(NULL == Name)
		return RMU_FILE_HANDLE_INVALID;

	if(!GetDirPath(DirTem, RMU_MAX_PATH, Name))
		return RMU_FILE_HANDLE_INVALID;
	
	if((INEXISTENCE == IsValidPath(DirTem)) && !Mkdir(DirTem))
	{
		printf("invalid dir %s\n", DirTem);
		return RMU_FILE_HANDLE_INVALID;
	}

	Fd = open(Name, Flag, -1);
	if(Fd<0)
	{
		printf("open file failed:%s\n", Name);
		return RMU_FILE_HANDLE_INVALID;
	}

	File = malloc(sizeof(RmuFile_t));
	assert(File);

	File->FileHdl = Fd;
	File->Pos = 0;
	strcpy(File->Name, Name);
	DlistAddTail(&s_FileManager, &File->List);
	return (RMU_FILE_HANDLE)	Fd;
}

void _Rmu_StorClose(RMU_FILE_HANDLE FileHandle)
{
	
}

int _Rmu_StorRead(RMU_FILE_HANDLE Handle, long long Type, void *Data, int Len)
{

}//long long : read ptr

int _Rmu_StorWrite(RMU_FILE_HANDLE Handle, long long Type, void *Data, int Len)
{

}

RMU_FILE_HANDLE _Rmu_GetFileSize(char *Name)
{

}

int GetDirFileInfoList()
{

}

FileStorage_t g_RmuStor = {
	"rmu",
	_Rmu_StorInit,
	_Rmu_StorDeInit,
	_Rmu_StorOpen,
	_Rmu_StorClose,
	_Rmu_StorRead,
	_Rmu_StorWrite,
	_Rmu_GetFileSize
};