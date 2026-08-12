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
	ReadLock(const ReadLock& other);
	ReadLock& operator=(const ReadLock& other);

private:
	ReadWriteMutexBase* m_mutex;
};