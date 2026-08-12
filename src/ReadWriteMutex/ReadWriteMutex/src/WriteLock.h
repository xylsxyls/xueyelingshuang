#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutexBase;

class ReadWriteMutexAPI WriteLock
{
public:
	WriteLock(ReadWriteMutexBase& mutex);
	~WriteLock();

private:
	WriteLock(const WriteLock& other);
	WriteLock& operator=(const WriteLock& other);

private:
	ReadWriteMutexBase* m_mutex;
};