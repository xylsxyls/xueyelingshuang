#include "FileReadWriteMutex.h"
#ifdef _MSC_VER
#include <windows.h>
#include <thread>
#elif __unix__
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#endif

std::string FileReadWriteMutex::s_tempDir = FileReadWriteMutex::tempDir();

FileReadWriteMutex::FileReadWriteMutex(const std::string& fileName)
{
	if (fileName.find_first_of('/') != -1 || fileName.find_first_of('\\') != -1)
	{
		m_fileName = fileName;
	}
	else
	{
		m_fileName = fileName + ".lock";
	}
	m_fileName = s_tempDir + m_fileName;
#ifdef __unix__
	system(("touch \"" + m_fileName + "\"").c_str());
	m_fd = open(m_fileName.c_str(), O_RDWR);
#endif
}

FileReadWriteMutex::~FileReadWriteMutex()
{
#ifdef __unix__
	if (m_fd != -1)
	{
		close(m_fd);
	}
#endif
}

void FileReadWriteMutex::read()
{
	write();
}

void FileReadWriteMutex::write()
{
#ifdef _MSC_VER
	while (true)
	{
		m_file = CreateFileA(m_fileName.c_str(), GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (LockFile(m_file, 0, 0, 0, 0) == TRUE)
		{
			break;
		}
		CloseHandle(m_file);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
#elif __unix__
	if(m_fd < 0)
	{
		printf("open file failed, m_fd = %d\n", m_fd);
		return;
	}
	bool isFailed = false;
	int32_t times = 0;
	int32_t timesCount = 10000;
	while (true)
	{
		int result = lockf(m_fd, F_LOCK, 0);
		if (result >= 0)
		{
			if (isFailed && times >= timesCount)
			{
				printf("success lockf function, result = %d, m_fileName = %s, m_fd = %d\n", result, m_fileName.c_str(), m_fd);
			}
			break;
		}
		++times;
		isFailed = true;
		if (times % timesCount == 0)
		{
			printf("warning lockf function failed times = %d, result = %d, m_fileName = %s, m_fd = %d\n", times, result, m_fileName.c_str(), m_fd);
		}
	}
#endif
}

void FileReadWriteMutex::unread()
{
	unwrite();
}

void FileReadWriteMutex::unwrite()
{
#ifdef _MSC_VER
	UnlockFile(m_file, 0, 0, 0, 0);
	CloseHandle(m_file);
	DeleteFileA(m_fileName.c_str());
#elif __unix__
	if (m_fd < 0)
	{
		printf("open file failed, m_fd = %d\n", m_fd);
		return;
	}
	int result = lockf(m_fd, F_ULOCK, 0);
	if (result < 0)
	{
		printf("unlock lockf function failed, result = %d\n", result);
		return;
	}
#endif
}

#ifdef __unix__
void FileReadWriteMutex::trywrite()
{
	if(m_fd < 0)
	{
		printf("open file failed, m_fd = %d\n", m_fd);
		return;
	}
	int result = lockf(m_fd, F_TLOCK, 0);
	if (result < 0)
	{
		printf("lockf function failed, result = %d\n", result);
		return;
	}
}
#endif


std::string FileReadWriteMutex::tempDir()
{
#ifdef _MSC_VER
	TCHAR szPath[MAX_PATH] = { 0 };
	GetTempPathA(MAX_PATH, szPath);
	return szPath;
#elif __unix__
	return "/tmp/";
#endif
}