#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C"{
#endif

#define MEM_SIZE(a) (sizeof(a)/sizeof((a)[0]))
typedef unsigned long RMU_FILE_HANDLE;

typedef struct{
	char *Name;
	int (*Init)(void);
	int (*DeInit)(void);
	RMU_FILE_HANDLE (*Open)(char *, unsigned);
	void (*Close)(RMU_FILE_HANDLE);
	int (*Read)(RMU_FILE_HANDLE, long long, void *, int);//long long : read ptr
	int (*Write)(RMU_FILE_HANDLE, long long, void *, int);//long long : write ptr
	RMU_FILE_HANDLE (*GetFileSize)(char *);
}FileStorage_t;

extern FileStorage_t g_RmuStor;

static FileStorage_t *s_StorList[] = {
	&g_RmuStor,
		
};

static inline FileStorage_t *GetFileStor(char *Name)
{
	int Idx;
	if(NULL == Name)
	{
		return NULL;
	}

	for(Idx=0; Idx<MEM_SIZE(s_StorList); Idx++)
	{
		if(!strcmp(s_StorList[Idx]->Name, Name))
		{
			return s_StorList[Idx];
		}
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif