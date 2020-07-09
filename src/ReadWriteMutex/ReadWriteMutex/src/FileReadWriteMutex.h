#pragma once
#include "ReadWriteMutexBase.h"
#include <string>

typedef void* HANDLE;

class ReadWriteMutexAPI FileReadWriteMutex : public ReadWriteMutexBase
{
public:
	FileReadWriteMutex(const std::string& fileName);

public:
	virtual void read();
	virtual void write();
	virtual void unread();
	virtual void unwrite();

private:
	HANDLE m_file;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_fileName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};