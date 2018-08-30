#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutex;

class ReadWriteMutexAPI ReadLock
{
public:
	ReadLock(ReadWriteMutex& mutex);
	~ReadLock();

private:
	ReadWriteMutex *m_mutex;
};