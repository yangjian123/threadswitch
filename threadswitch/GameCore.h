
#ifndef __GAMECORE_H__
#define __GAMECORE_H__


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

#define MAXGMTHREAD	0x100


#define GMTHREAD_CREATE		0x01
#define GMTHREAD_READAY		0x02
#define GMTHREAD_RUNING		0x04
#define GMTHREAD_SLEEP		0x08
#define GMTHREAD_SUSPEND	0x10
#define GMTHREAD_EXIT	0x100

typedef struct
{
	char *name;
	int Flags;

	int SuspendCount;
	int SleepMillisecondDot;
	void *WindowsInitialStack;
	void *WindowsKernelStack;

	void *InitialStack;
	void *StackLimit;
	void *KernelStack;
	
	void *lpParameter;
	void (*func)(void *lpParameter);

} GMThread_t;


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------

extern GMThread_t GMThreadList[MAXGMTHREAD];

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------


void IdleGMThread(void *lpParameter);
void InitWindowsStack(GMThread_t *GMThreadp);

void GMThreadStartup(GMThread_t *GMThreadp);
void initGMThread(GMThread_t *GMThreadp,char *name,void (*func)(void *lpParameter),void *lpParameter);
int RegisterGMThread(char *name,void (*func)(void *lpParameter),void *lpParameter);
void Scheduling(void);

void GMSleep(int Milliseconds,int Freeze = 0);

void GamePolling(int Milliseconds);

int SuspendGMThread(int h);
int ResumeGMThread(int h);

void PickThing(void *lpParameter);
void PlusBlood(void *lpParameter);
void FeedingPets(void *lpParameter);
void FindWay(void *lpParameter);
void Command(void *lpParameter);


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#endif //#ifndef __GAMECORE_H__

