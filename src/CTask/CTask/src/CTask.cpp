#include "CTask.h"

CTask::CTask(){
	node = 0;
	ifExceptionRun = 0;
}

bool CTask::CheckRun(){
	return true;
}

bool CTask::Run(){
	return true;
}

bool CTask::SuspendRun(){
	return true;
}

void CTask::ExceptionRun(int exception){
	;
}

int CTask::CheckException(){
	return true;
}