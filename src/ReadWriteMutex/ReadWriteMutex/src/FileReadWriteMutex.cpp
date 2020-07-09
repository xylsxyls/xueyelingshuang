#include "FileReadWriteMutex.h"
#include <windows.h>
#include <thread>

FileReadWriteMutex::FileReadWriteMutex(const std::string& fileName)
{
	m_fileName = fileName + ".lock";
}

void FileReadWriteMutex::read()
{
	write();
}

void FileReadWriteMutex::write()
{
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
}

void FileReadWriteMutex::unread()
{
	unwrite();
}

void FileReadWriteMutex::unwrite()
{
	UnlockFile(m_file, 0, 0, 0, 0);
	CloseHandle(m_file);
	DeleteFileA(m_fileName.c_str());
}