#include <SDKDDKVer.h>
#include "CStopWatch.h"

CStopWatch::CStopWatch(){
	time = GetTickCount();
	StopTime = 0;
	RunTime = 0;
	StopOrRun = 1;
	handle = CreateEvent(NULL, TRUE, FALSE, NULL);
	ReturnHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
	CountDownSeconds = 0;
	//pMutex = new CMutex;
	//pMutexDo = new CMutex;
}

CStopWatch::~CStopWatch(){
	delete pMutex;
	delete pMutexDo;
};

unsigned long CStopWatch::GetWatchTime(){
	if(StopOrRun == 0) return StopTime       - time;
	else               return GetTickCount() - time;
}

void CStopWatch::SetWatchTime(unsigned long SetTime){
	time = time - (SetTime - GetWatchTime());
}

void CStopWatch::Stop(){
	if(StopOrRun == 0) return;
	StopOrRun = 0;
	StopTime = GetTickCount();
}

typedef struct StopThreadPac{
	CStopWatch* pThis;
	unsigned long* pCountDownSeconds;
	void *pDo;
	HANDLE handle;
	HANDLE ReturnHandle;
	CMutex* pMutexDo;
	BOOL nDelete;
}StopThreadPac;

DWORD WINAPI StopThread(LPVOID lparam){
	StopThreadPac package = *(StopThreadPac *)lparam;
	delete (StopThreadPac *)lparam;

	//不可以用循环，这样消耗cpu太高
	ResetEvent(package.handle);
	//在Reset之后才可以让函数返回
	SetEvent(package.ReturnHandle);
	int nRet = WaitForSingleObject(package.handle,*(package.pCountDownSeconds));
	//如果被重置则继续循环
	if(nRet == WAIT_OBJECT_0){
		return 0;
	}
	else if(nRet == WAIT_TIMEOUT){
		//package.pMutexDo->Lock();
		package.pThis->Do(package.pDo,&(package.nDelete));
		if(package.nDelete == 1) return 0;
		//package.pMutexDo->Unlock();
	}
    else;// AfxMessageBox(L"信号出错");

	return 0;
}

void CStopWatch::CountDown(unsigned long CountDownSeconds,void *pDo){
	ResetEvent(ReturnHandle);
	//pMutex->Lock();
	this->CountDownSeconds = CountDownSeconds;
	//pMutex->Unlock();
	StopThreadPac* ppackage = new StopThreadPac;
	ppackage->pCountDownSeconds = &this->CountDownSeconds;
	ppackage->pThis = this;
	ppackage->pDo = pDo;
	ppackage->handle = handle;
	ppackage->ReturnHandle = ReturnHandle;
	ppackage->pMutexDo = pMutexDo;
	ppackage->nDelete = 0;
	SetEvent(handle);
	DWORD ThreadID = NULL;
	Create_Thread(StopThread,ppackage,ThreadID);
	WaitForSingleObject(ReturnHandle,INFINITE);
	return;
}

void CStopWatch::Do(void *pDo,BOOL* nDelete){
	return;
}

void CStopWatch::Run(){
	if(StopOrRun == 1) return;
	StopOrRun = 1;
	RunTime = GetTickCount();
	time = time + RunTime - StopTime;
	return;
}

int CStopWatch::GetHour(){
    return GetWatchTime() / 3600000;
}

int CStopWatch::GetMinute(){
	return (GetWatchTime() - GetHour() * 3600000) / 60000;
}

double CStopWatch::GetSeconds(){
	return (GetWatchTime() - GetHour() * 3600000 - GetMinute() * 60000) / 1000.0;
}