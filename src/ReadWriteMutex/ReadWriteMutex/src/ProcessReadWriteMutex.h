#pragma once
#include "ReadWriteMutexMacro.h"
#include <string>
#include "ReadWriteMutexBase.h"

#ifdef _MSC_VER
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
	bool trywrite();
	
private:
	HANDLE m_hLock;
};
#elif __linux__
class ReadWriteMutexAPI ProcessReadWriteMutex : public FileReadWriteMutex
{

};
#endif