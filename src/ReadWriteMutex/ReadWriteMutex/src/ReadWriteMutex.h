#pragma once
#include "ReadWriteMutexMacro.h"
#include "ReadWriteMutexBase.h"

struct _RTL_SRWLOCK;
typedef _RTL_SRWLOCK RTL_SRWLOCK;
typedef RTL_SRWLOCK SRWLOCK;

class ReadWriteMutexAPI ReadWriteMutex : public ReadWriteMutexBase
{
public:
	ReadWriteMutex();
	~ReadWriteMutex();

public:
	virtual void read();
	virtual void unread();
	virtual void write();
	virtual void unwrite();

private:
	SRWLOCK* m_mutex;
};