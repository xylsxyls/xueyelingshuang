#include "CTask.h"
#include "CSystem/CSystemAPI.h"

CTask::CTask(){
	node = 0;
	ifExceptionRun = 0;
	uuid = CSystem::uuid();
}

int CTask::initRun(){
	return true;
}

int CTask::Run(){
	return true;
}

void CTask::SuspendRun(){
	return;
}

void CTask::ExceptionRun(int exception){
	;
}

int CTask::CheckException(){
	return true;
}

void CTask::SetEmpty(){
	node = -1;
}

bool CTask::isValid(){
	return node != -1;
}