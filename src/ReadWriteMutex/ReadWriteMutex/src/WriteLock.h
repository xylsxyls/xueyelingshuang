#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutexBase;

class ReadWriteMutexAPI WriteLock
{
public:
	WriteLock(ReadWriteMutexBase& mutex);
	~WriteLock();

private:
	ReadWriteMutexBase* m_mutex;
};