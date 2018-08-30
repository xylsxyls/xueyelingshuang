#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutex;

class ReadWriteMutexAPI WriteLock
{
public:
	WriteLock(ReadWriteMutex& mutex);
	~WriteLock();

private:
	ReadWriteMutex* m_mutex;
};