
#include "stdafx.h"
#include "stdio.h"
#include "windows.h"

#include "GameCore.h"

#define _SELF		"GameCore"


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

int FreezeMilliseconds = 0;
int CurrentThreadindex = 0;
GMThread_t GMThreadList[MAXGMTHREAD] = { NULL,0 };

#define USEWINDOWSSTACK	0
#define GMTHREADSTACKSIZE 0x80000

void *WindowsStackLimit = NULL;

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void backupWindowsStack(GMThread_t *SrcGMThreadp)
{
	int backupLength;
	backupLength = (char*)SrcGMThreadp->WindowsInitialStack-(char*)SrcGMThreadp->WindowsKernelStack;
	SrcGMThreadp->KernelStack = (char*)SrcGMThreadp->InitialStack-backupLength;
	memcpy(SrcGMThreadp->KernelStack,SrcGMThreadp->WindowsKernelStack,backupLength);

	return ;
}

void resumeWindowsStack(GMThread_t *DstGMThreadp)
{
	int restoreLength;
	restoreLength = (char*)DstGMThreadp->InitialStack-(char*)DstGMThreadp->KernelStack;

	//DstGMThreadp->WindowsInitialStack = (char*)DstGMThreadp->WindowsKernelStack+restoreLength;
	//DstGMThreadp->WindowsKernelStack = (char*)DstGMThreadp->WindowsInitialStack-restoreLength;

	memcpy(DstGMThreadp->WindowsKernelStack,DstGMThreadp->KernelStack,restoreLength);

	return ;
}

__declspec(naked) void SwitchContext(GMThread_t *SrcGMThreadp,GMThread_t *DstGMThreadp)
{
	__asm
	{
		push ebp
		mov ebp,esp
		//sub esp,__LOCAL_SIZE
		push edi
		push esi
		push ebx
		push ecx
		push edx
		push eax
		
		mov esi,SrcGMThreadp
		mov edi,DstGMThreadp
		
		//---------------------------------
		
#if (USEWINDOWSSTACK==1)
		mov [esi+GMThread_t.WindowsKernelStack],esp
		push esi
		call backupWindowsStack
		add esp,4
		//---------------------------------
	
		mov esp,[edi+GMThread_t.WindowsKernelStack]
		push edi
		call resumeWindowsStack
		add esp,4
#else
		//---------------------------------
		mov [esi+GMThread_t.KernelStack],esp
		//½›µä¾Q³ÌÇÐ“Q
		mov esp,[edi+GMThread_t.KernelStack]
#endif
		
		pop eax
		pop edx
		pop ecx
		pop ebx
		pop esi
		pop edi
		//add esp,__LOCAL_SIZE
		pop ebp
		ret
	}
}


void GMThreadStartup(GMThread_t *GMThreadp)
{
	GMThreadp->func(GMThreadp->lpParameter);

	GMThreadp->Flags = GMTHREAD_EXIT;
	Scheduling();

	return ;
}

void InitWindowsStack(GMThread_t *GMThreadp)
{
	GMThreadp->WindowsInitialStack = GMTHREADSTACKSIZE-0x10+(char*)WindowsStackLimit;
	GMThreadp->WindowsKernelStack = (char*)GMThreadp->WindowsInitialStack-
		((char*)GMThreadp->InitialStack - (char*)GMThreadp->KernelStack);

	return ;
}

void IdleGMThread(void *lpParameter)
{
#if (USEWINDOWSSTACK==1)
	
	int i;
	
	if (WindowsStackLimit==NULL)
	{
		WindowsStackLimit = lpParameter;
	}
	if (WindowsStackLimit!=lpParameter)
	{
		return ;
	}
	
	GMThreadList[0].WindowsInitialStack = lpParameter;
	for (i=1;GMThreadList[i].name;i++)
	{
		InitWindowsStack(&GMThreadList[i]);
	}
	
#endif
	
	Scheduling();
	return ;
}

void PushStack(unsigned int **Stackpp,unsigned int v)
{
	*Stackpp -= 1;
	**Stackpp = v;

	return ;
}

void initGMThread(GMThread_t *GMThreadp,char *name,void (*func)(void *lpParameter),void *lpParameter)
{
	unsigned char *StackPages;
	unsigned int *StackDWORDParam;

	GMThreadp->Flags = GMTHREAD_CREATE;
	GMThreadp->name = name;
	GMThreadp->func = func;
	GMThreadp->lpParameter = lpParameter;

#if (USEWINDOWSSTACK==1)
	GMThreadp->SuspendCount = 0;
	GMThreadp->SleepMillisecondDot = 0;
#endif

	StackPages = (unsigned char*)VirtualAlloc(NULL,GMTHREADSTACKSIZE,MEM_COMMIT,PAGE_READWRITE);
	memset(StackPages,0,GMTHREADSTACKSIZE);
	GMThreadp->InitialStack = (StackPages+GMTHREADSTACKSIZE);
	GMThreadp->StackLimit = StackPages;
	
	StackDWORDParam = (unsigned int*)GMThreadp->InitialStack;
	
	PushStack(&StackDWORDParam,(unsigned int)GMThreadp);
	PushStack(&StackDWORDParam,(unsigned int)9);
	PushStack(&StackDWORDParam,(unsigned int)GMThreadStartup);
	PushStack(&StackDWORDParam,5); //push ebp
	PushStack(&StackDWORDParam,7); //push edi
	PushStack(&StackDWORDParam,6); //push esi
	PushStack(&StackDWORDParam,3); //push ebx
	PushStack(&StackDWORDParam,2); //push ecx
	PushStack(&StackDWORDParam,1); //push edx
	PushStack(&StackDWORDParam,0); //push eax

	GMThreadp->KernelStack = StackDWORDParam;

#if  (USEWINDOWSSTACK==1)
	InitWindowsStack(GMThreadp);
#endif //  (USEWINDOWSSTACK==1)

	GMThreadp->Flags = GMTHREAD_READAY;

	return ;
}

int RegisterGMThread(char *name,void (*func)(void *lpParameter),void *lpParameter)
{
	int i;

	if (GMThreadList[0].name==NULL)
	{
		initGMThread(&GMThreadList[0],"IDLE GM Thread",IdleGMThread,NULL);
	}

	for (i=1;GMThreadList[i].name;i++)
	{
		if (0==stricmp(GMThreadList[i].name,name))
		{
			break;
		}
	}
	initGMThread(&GMThreadList[i],name,func,lpParameter);

	return (i|0x55AA0000);
}

int SuspendGMThread(int h)
{
	int Susp;

	Susp = 0;
	if ((h&0xFFFF0000)==0x55AA0000)
	{
		GMThreadList[h&0xFF].Flags = GMTHREAD_SUSPEND;
		GMThreadList[h&0xFF].SuspendCount += 1;
		Susp = GMThreadList[h&0xFF].SuspendCount;
	}

	return Susp;
}

int ResumeGMThread(int h)
{
	int Susp;
	
	Susp = -1;
	if ((h&0xFFFF0000)==0x55AA0000)
	{
		GMThreadList[h&0xFF].SuspendCount -= 1;
		Susp = GMThreadList[h&0xFF].SuspendCount;
		if (Susp==0)
		{
			GMThreadList[h&0xFF].Flags = GMTHREAD_READAY;
		}
	}
	
	return Susp;	
}

void Scheduling(void)
{
	int i;
	int TickCount;
	GMThread_t *SrcGMThreadp;
	GMThread_t *DstGMThreadp;

	TickCount = GetTickCount()-FreezeMilliseconds;

	SrcGMThreadp = &GMThreadList[CurrentThreadindex];
	
	DstGMThreadp = &GMThreadList[0];
	for (i=1;GMThreadList[i].name;i++)
	{
		if (GMThreadList[i].Flags&GMTHREAD_SLEEP)
		{
			if (TickCount>GMThreadList[i].SleepMillisecondDot)
			{
				GMThreadList[i].Flags = GMTHREAD_READAY;
			}
		}

		if ((GMThreadList[i].Flags&GMTHREAD_READAY))
		{
			DstGMThreadp = &GMThreadList[i];
			break;
		}
	}
	
	CurrentThreadindex = DstGMThreadp-GMThreadList;
	SwitchContext(SrcGMThreadp,DstGMThreadp);
	
	return ;
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

void GMSleep(int Milliseconds,int Freeze)
{
	GMThread_t *GMThreadp;
	GMThreadp = &GMThreadList[CurrentThreadindex];

	if (Freeze)
	{
		FreezeMilliseconds += Milliseconds;
	}

	if ((GMThreadp->Flags&GMTHREAD_SUSPEND)==0)
	{
		GMThreadp->SleepMillisecondDot = (GetTickCount()-FreezeMilliseconds)+Milliseconds;
		GMThreadp->Flags = GMTHREAD_SLEEP;
	}

	Scheduling();
	return ;
}

void GamePolling(int Milliseconds)
{
	unsigned char StackPage[GMTHREADSTACKSIZE];
	memset(StackPage,0,GMTHREADSTACKSIZE);
	IdleGMThread(StackPage);

	return ;
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
void vmmprint(char *f,int l,char *fmt, ...)
{
	int ret;
	char buffer[0x100];
	va_list args;
	
	//----------------------------------
	va_start(args, fmt);
	_snprintf(buffer,0x80,"F[%s(%d)]:",f,l);
	ret = _vsnprintf(buffer+strlen(buffer), 0x100-strlen(buffer), fmt, args);
	if (ret == -1)
	{
		strcpy(buffer, "vmmprint: error.");
	}
	//----------------------------------
	//printf("%s",buffer);
	OutputDebugString(buffer);
	
	return ;
}

void PickThing(void *lpParameter)
{
	for (;;)
	{
		vmmprint(_SELF,__LINE__,"PickThing \n");
		GMSleep(3000);
	}
	return ;
}


void PlusBlood(void *lpParameter)
{
	while (true)
	{
		printf("plusblood threading called\r\n..");
		GMSleep(200);
	}
	
	return ;
}

void FeedingPets(void *lpParameter)
{
	while (true)
	{
		printf("FeedingPets threading called\r\n..");
		GMSleep(200);
	}
	return ;
}

void FindWay(void *lpParameter)
{
	while (true)
	{
		printf("FindWay threading called\r\n..");
		GMSleep(200);
	}
	
	return ;
}

void Command(void *lpParameter)
{
	while (true)
	{
		printf("Command threading called\r\n..");
		GMSleep(200);
	}
	
	return ;
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	RegisterGMThread("Plus blood",PlusBlood,NULL);
	RegisterGMThread("Feeding pets",FeedingPets,NULL);
	RegisterGMThread("FindWay",FindWay,NULL);
	RegisterGMThread("Command",Command,NULL);

	for (;;)
	{
		Sleep(20);
		GamePolling(20);
	}

	return 0;
}

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

