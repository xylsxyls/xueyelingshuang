#pragma once
#include "ReadWriteMutexMacro.h"
#include <string>
#include "ReadWriteMutexBase.h"

typedef void* HANDLE;

class ReadWriteMutexAPI ProcessReadWriteMutex : public ReadWriteMutexBase
{
public:
	ProcessReadWriteMutex(const std::string& name);
	virtual ~ProcessReadWriteMutex();

public:
	virtual void read();
	virtual void unread();
	virtual void write();
	virtual void unwrite();
	
private:
	HANDLE m_hLock;
};