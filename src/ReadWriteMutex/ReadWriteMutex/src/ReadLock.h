#pragma once
#include "ReadWriteMutexMacro.h"
#include "ReadWriteMutexBase.h"

class ReadWriteMutexBase;

class ReadWriteMutexAPI ReadLock
{
public:
	ReadLock(ReadWriteMutexBase& mutex);
	~ReadLock();

private:
	ReadWriteMutexBase* m_mutex;
};