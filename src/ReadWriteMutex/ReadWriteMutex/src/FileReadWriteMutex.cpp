#include "FileReadWriteMutex.h"
#ifdef _MSC_VER
#include <windows.h>
#include <thread>
#elif __unix__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdio.h>

std::string FileReadWriteMutex::s_tempDir = FileReadWriteMutex::tempDir();

FileReadWriteMutex::FileReadWriteMutex(const std::string& filePath) :
#ifdef _MSC_VER
m_file(nullptr),
#elif __unix__
m_fd(-1),
#endif
m_filePath(""),
m_isName(false),
m_isLocked(false)
{
	if (filePath.find_first_of('/') != std::string::npos || filePath.find_first_of('\\') != std::string::npos)
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
	m_fd = open(m_filePath.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif
}

FileReadWriteMutex::~FileReadWriteMutex()
{
	if (m_isLocked)
	{
		unwrite();
	}
#ifdef __unix__
	if (m_fd != -1)
	{
		close(m_fd);
		m_fd = -1;
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
	if (m_isLocked)
	{
		return;
	}
	int32_t count = 10000;
	while (count-- != 0)
	{
		HANDLE file = CreateFileA(m_filePath.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		OVERLAPPED overlapped = {};
		if (LockFileEx(file, LOCKFILE_EXCLUSIVE_LOCK, (DWORD)0, (DWORD)1, (DWORD)0, &overlapped) == TRUE)
		{
			m_file = file;
			m_isLocked = true;
			return;
		}
		DWORD lastError = ::GetLastError();
		CloseHandle(file);
		if (count == 0)
		{
			printf("lock file error, lastError = %d\n", static_cast<int32_t>(lastError));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
#elif __unix__
	if(m_fd < 0)
	{
		printf("open file failed, m_fd = %d\n", m_fd);
		return;
	}
	if (m_isLocked)
	{
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
			m_isLocked = true;
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
	if (!m_isLocked || m_file == nullptr)
	{
		return;
	}
	OVERLAPPED overlapped = {};
	UnlockFileEx(m_file, (DWORD)0, (DWORD)1, (DWORD)0, &overlapped);
	CloseHandle(m_file);
	m_file = nullptr;
	m_isLocked = false;
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
	if (!m_isLocked)
	{
		return;
	}
	int result = lockf(m_fd, F_ULOCK, 0);
	if (result < 0)
	{
		printf("unlock lockf function failed, result = %d\n", result);
		return;
	}
	m_isLocked = false;
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
	if (m_isLocked)
	{
		return;
	}
	int result = lockf(m_fd, F_TLOCK, 0);
	if (result < 0)
	{
		printf("lockf function failed, result = %d\n", result);
		return;
	}
	m_isLocked = true;
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