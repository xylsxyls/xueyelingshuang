#pragma once
#include "CStopWatch.h"
#include <Windows.h>

CStopWatch::CStopWatch(){
	time = GetTickCount();
	StopTime = 0;
	RunTime = 0;
	StopOrRun = 1;
	StopSecondsTime = 0;
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

DWORD WINAPI StopThread(LPVOID lparam){
	CStopWatch *p_StopWatch = (CStopWatch *)lparam;
	unsigned long temp = GetTickCount();

	while(GetTickCount() - temp <= p_StopWatch->StopSecondsTime);

	p_StopWatch->Run();
	p_StopWatch->StopOrRun = 1;
	p_StopWatch->StopSecondsTime = 0;
	return 0;
}

void CStopWatch::Stop(unsigned long StopSecondsTime){
	Stop();
	this->StopSecondsTime = StopSecondsTime;
	DWORD ThreadID = NULL;
	Create_Thread(StopThread,this,ThreadID);
	//StopThread(this);
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