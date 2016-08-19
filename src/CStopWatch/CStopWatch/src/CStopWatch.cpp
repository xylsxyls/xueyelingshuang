#pragma once
#include "CStopWatch.h"
#include <Windows.h>

CStopWatch::CStopWatch(){
	time = GetTickCount();
	StopTime = 0;
	RunTime = 0;
	StopOrRun = 1;
}

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
	unsigned long nStopSeconds;
	void *pDo;
}StopThreadPac;

DWORD WINAPI StopThread(LPVOID lparam){
	StopThreadPac package = *(StopThreadPac *)lparam;
	delete (StopThreadPac *)lparam;

	//不可以用循环，这样消耗cpu太高
	Sleep(package.nStopSeconds);
	package.pThis->Run();
	package.pThis->Do(package.pDo);
	return 0;
}

void CStopWatch::Stop(unsigned long nStopSeconds,void *pDo){
	Stop();
	StopThreadPac* ppackage = new StopThreadPac;
	ppackage->nStopSeconds = nStopSeconds;
	ppackage->pThis = this;
	ppackage->pDo = pDo;
	DWORD ThreadID = NULL;
	Create_Thread(StopThread,ppackage,ThreadID);
}

void CStopWatch::Do(void *pDo){
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
	return GetWatchTime()/3600000;
}

int CStopWatch::GetMinute(){
	return (GetWatchTime() - GetHour() * 3600000) / 60000;
}

double CStopWatch::GetSeconds(){
	return (GetWatchTime() - GetHour() * 3600000 - GetMinute() * 60000) / 1000.0;
}