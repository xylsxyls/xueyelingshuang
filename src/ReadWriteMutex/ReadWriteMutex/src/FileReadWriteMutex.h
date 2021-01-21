#pragma once
#include "ReadWriteMutexBase.h"
#include <string>

#ifdef _MSC_VER
typedef void* HANDLE;
#endif

class ReadWriteMutexAPI FileReadWriteMutex : public ReadWriteMutexBase
{
public:
	FileReadWriteMutex(const std::string& fileName);

public:
	virtual void read();
	virtual void write();
	virtual void unread();
	virtual void unwrite();

#ifdef __unix__
	void trywrite();
#endif

	static std::string tempDir();

private:
#ifdef _MSC_VER
	HANDLE m_file;
#elif __unix__
	int m_fd;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	static std::string s_tempDir;
	std::string m_fileName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};