#include "stdafx.h"
#include "ATask.h"
#include "CKeyboard/CKeyboardAPI.h"

ATask::ATask():
m_exit(false)
{

}

void ATask::DoTask()
{
	while (!m_exit)
	{
		CKeyboard::KeyPress('C', 0);
		Sleep(200);
	}
}

void ATask::StopTask()
{
	m_exit = true;
}

bool ATask::ReExecute()
{
	return true;
}

CTask* ATask::Clone()
{
	return new ATask;
}
