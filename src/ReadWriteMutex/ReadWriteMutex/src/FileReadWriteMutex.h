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

#ifdef __linux__
	void trywrite();
#endif

#ifdef _MSC_VER
	static std::string tempDir();
#endif

private:
#ifdef _MSC_VER
	HANDLE m_file;
#elif __linux__
	int m_fd;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
#ifdef _MSC_VER
	static std::string s_tempDir;
#endif
	std::string m_fileName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};