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

FileReadWriteMutex::FileReadWriteMutex(const std::string& filePath)
{
	if (filePath.find_first_of('/') != -1 || filePath.find_first_of('\\') != -1)
	{
		m_filePath = filePath;
		m_isName = false;
	}
	else
	{
		m_filePath = s_tempDir + filePath + ".lock";
		m_isName = true;
	}
#ifdef __unix__
	system(("touch \"" + m_filePath + "\"").c_str());
	m_fd = open(m_filePath.c_str(), O_RDWR);
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
	int32_t count = 10000;
	while (count-- != 0)
	{
		m_file = CreateFileA(m_filePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		OVERLAPPED oapped;
		if (LockFileEx(m_file, LOCKFILE_EXCLUSIVE_LOCK, (DWORD)0, (DWORD)1, (DWORD)0, &oapped) == TRUE)
		{
			break;
		}
		CloseHandle(m_file);
		//windows下不应该打印这句
		printf("lock file error, lastError = %d\n", (int32_t)::GetLastError());
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
				printf("success lockf function, result = %d, m_filePath = %s, m_fd = %d\n", result, m_filePath.c_str(), m_fd);
			}
			break;
		}
		++times;
		isFailed = true;
		if (times % timesCount == 0)
		{
			printf("warning lockf function failed times = %d, result = %d, m_filePath = %s, m_fd = %d\n", times, result, m_filePath.c_str(), m_fd);
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
	OVERLAPPED oapped;
	UnlockFileEx(m_file, (DWORD)0, (DWORD)1, (DWORD)0, &oapped);
	CloseHandle(m_file);
	if (m_isName)
	{
		DeleteFileA(m_filePath.c_str());
	}
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