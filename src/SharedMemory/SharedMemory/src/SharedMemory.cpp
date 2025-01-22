#include "SharedMemory.h"
#ifdef __unix__
#include <string.h>
#include <sys/shm.h>
#else
#include <Windows.h>
#include <psapi.h>
#include <TCHAR.H>
#include <strsafe.h>
#endif

SharedMemory::SharedMemory(const std::string& name, uint32_t size, bool isAllowExist):
#ifdef _MSC_VER
m_memoryHandle(nullptr),
m_readMemoryPtr(nullptr),
m_writeMemoryPtr(nullptr),
#elif __unix__
m_shmid(-1),
#endif
m_memoryPtr(nullptr),
m_memoryName(name),
m_size(0)
{
	if (size != 0)
	{
#ifdef _MSC_VER
		m_memoryHandle = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name.empty() ? nullptr : name.c_str());
#elif __unix__
		std::string fileName = "/dev/shm/SharedMemory." + name;
		system(("touch \"" + fileName + "\"").c_str());
		m_shmid = shmget(ftok(fileName.c_str(), 0), size, (isAllowExist ? (0666 | IPC_CREAT) : (0666 | IPC_EXCL | IPC_CREAT)));
#endif
		m_size = size;
	}
}

SharedMemory::SharedMemory(const SharedMemory& other)
{

}

SharedMemory SharedMemory::operator=(const SharedMemory& other)
{
	return *this;
}

SharedMemory::~SharedMemory()
{
	close();
#ifdef _MSC_VER
	if (m_memoryHandle != nullptr)
	{
		::CloseHandle(m_memoryHandle);
		m_memoryHandle = nullptr;
	}
#elif __unix__
	if(m_size != 0 && m_shmid != -1)
	{
		if(shmctl(m_shmid, IPC_RMID, 0) == -1)
		{
			printf("shmctl(IPC_RMID) failed, mapName = %s\n", m_memoryName.c_str());
		}
		system(("rm \"/dev/shm/SharedMemory." + m_memoryName + "\"").c_str());
	}
#endif
}

bool SharedMemory::isFailed()
{
#ifdef _MSC_VER
	return (m_memoryHandle == nullptr);
#elif __unix__
	return (m_shmid == -1);
#endif
}

uint32_t SharedMemory::size()
{
	return m_size;
}

uint32_t SharedMemory::realSize()
{
#ifdef _MSC_VER
	if (m_memoryHandle == nullptr)
	{
		open(true);
	}
	if (m_memoryHandle == nullptr)
	{
		return 0;
	}
	MEMORY_BASIC_INFORMATION mem_info;
	::VirtualQuery(::MapViewOfFile(m_memoryHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0), &mem_info, sizeof(mem_info));
	return mem_info.RegionSize;
#elif __unix__
	if(m_shmid == -1)
	{
		open(true);
	}
	if(m_shmid == -1)
	{
		return 0;
	}
	struct shmid_ds shmbuffer;
	//读共享内存结构struct shmid_ds
	shmctl(m_shmid, IPC_STAT, &shmbuffer);
	return shmbuffer.shm_segsz;
#endif
}

std::string SharedMemory::mapName()
{
	return m_memoryName;
}

void* SharedMemory::memory()
{
	return m_memoryPtr;
}

void* SharedMemory::readWithoutLock()
{
#ifdef _MSC_VER
	if (m_readMemoryPtr != nullptr)
	{
		return m_memoryPtr = m_readMemoryPtr;
	}
	if (m_memoryHandle == nullptr)
	{
		open(true);
	}
	if (m_memoryHandle == nullptr)
	{
		return nullptr;
	}
	m_readMemoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_READ, 0, 0, 0);
	m_memoryPtr = m_readMemoryPtr;
	return m_memoryPtr;
#elif __unix__
	return writeWithoutLock();
#endif
}

void* SharedMemory::writeWithoutLock()
{
#ifdef _MSC_VER
	if (m_writeMemoryPtr != nullptr)
	{
		return m_memoryPtr = m_writeMemoryPtr;
	}
	if (m_memoryHandle == nullptr)
	{
		open(false);
	}
	if (m_memoryHandle == nullptr)
	{
		return nullptr;
	}
	m_writeMemoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	m_memoryPtr = m_writeMemoryPtr;
#elif __unix__
	if (m_memoryPtr != nullptr)
	{
		return m_memoryPtr;
	}
	if (m_shmid == -1)
	{
		open(false);
	}
	if (m_shmid == -1)
	{
		return nullptr;
	}
	m_memoryPtr = shmat(m_shmid, (void*)0, 0);
	if ((size_t)m_memoryPtr == -1)
	{
		m_memoryPtr = nullptr;
		return nullptr;
	}
#endif
	return m_memoryPtr;
}

//bool SharedMemory::trywrite()
//{
//	if (!m_processReadWriteMutex.trywrite())
//	{
//		return false;
//	}
//	open(false);
//	if (m_memoryHandle == nullptr)
//	{
//		m_processReadWriteMutex.unwrite();
//		return false;
//	}
//	m_memoryPtr = m_writeMemoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
//	return true;
//}

void SharedMemory::close()
{
#ifdef _MSC_VER
	if (m_readMemoryPtr != nullptr)
	{
		::UnmapViewOfFile(m_readMemoryPtr);
		m_readMemoryPtr = nullptr;
	}

	if (m_writeMemoryPtr != nullptr)
	{
		::UnmapViewOfFile(m_writeMemoryPtr);
		m_writeMemoryPtr = nullptr;
	}
#elif __unix__
	if(m_memoryPtr != nullptr)
	{
		if (shmdt(m_memoryPtr) == -1)
		{
			printf("shmdt failed\n");
		}
	}
#endif
	m_memoryPtr = nullptr;
}

void SharedMemory::clear()
{
	::memset(writeWithoutLock(), 0, size());
}

#ifdef _MSC_VER
std::string SharedMemory::mapName(HANDLE memoryHandle, int32_t bufferSize)
{
	//BOOL bSuccess = FALSE;
	TCHAR pszFilename[MAX_PATH + 1];
	HANDLE hFileMap;

	// Get the file size.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(memoryHandle, &dwFileSizeHi);

	if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
	{
		//_tprintf(TEXT("Cannot map a file with a length of zero.\n"));
		return "";
	}

	// Create a file mapping object.
	hFileMap = CreateFileMapping(memoryHandle,
		NULL,
		PAGE_READONLY,
		0,
		1,
		NULL);

	if (hFileMap)
	{
		// Create a file mapping to get the file name.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem)
		{
			if (GetMappedFileName(GetCurrentProcess(),
				pMem,
				pszFilename,
				MAX_PATH))
			{

				// Translate path with device name to drive letters.
				TCHAR* szTemp = new TCHAR[bufferSize];
				szTemp[0] = '\0';

				if (GetLogicalDriveStrings(bufferSize - 1, szTemp))
				{
					TCHAR szName[MAX_PATH];
					TCHAR szDrive[3] = TEXT(" :");
					BOOL bFound = FALSE;
					TCHAR* p = szTemp;

					do
					{
						// Copy the drive letter to the template string
						*szDrive = *p;

						// Look up each device name
						if (QueryDosDevice(szDrive, szName, MAX_PATH))
						{
							size_t uNameLen = _tcslen(szName);

							if (uNameLen < MAX_PATH)
							{
								bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
									&& *(pszFilename + uNameLen) == _T('\\');

								if (bFound)
								{
									// Reconstruct pszFilename using szTempFile
									// Replace device path with DOS path
									TCHAR szTempFile[MAX_PATH];
									StringCchPrintf(szTempFile,
										MAX_PATH,
										TEXT("%s%s"),
										szDrive,
										pszFilename + uNameLen);
									StringCchCopyN(pszFilename, MAX_PATH + 1, szTempFile, _tcslen(szTempFile));
								}
							}
						}

						// Go to the next NULL character.
						while (*p++);
					} while (!bFound && *p); // end of string
				}

				delete[] szTemp;
			}
			//bSuccess = TRUE;
			UnmapViewOfFile(pMem);
		}

		CloseHandle(hFileMap);
	}
	//_tprintf(TEXT("File name is %s\n"), pszFilename);
	return pszFilename;
}
#endif

void SharedMemory::open(bool bReadOnly)
{
#ifdef _MSC_VER
	m_memoryHandle = ::OpenFileMapping(bReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, FALSE, m_memoryName.empty() ? NULL : m_memoryName.c_str());
#elif __unix__
	key_t key = ftok(("/dev/shm/SharedMemory." + m_memoryName).c_str(), 0);
	if (key == -1)
	{
		return;
	}
	m_shmid = shmget(key, 0, 0666 | IPC_CREAT);
#endif
}

////写
//int32_t main()
//{
//	SharedMemory mem("mmsa", 4095);
//	int32_t size = mem.size();
//	{
//		WriteLock writelock(mem);
//		void* ptr = mem.memory();
//		memcpy(ptr, "121212\0", 7);
//	}
//	getchar();
//	return 0;
//}
//
////读
//int32_t main()
//{
//	SharedMemory mem("mmsa");
//	int32_t size = mem.size();
//	{
//		ReadLock readlock(mem);
//		void* ptr = mem.memory();
//		printf("%s\n", ptr);
//	}
//	getchar();
//	return 0;
//}