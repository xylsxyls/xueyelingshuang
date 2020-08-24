#pragma once
#ifdef _WIN32
#define _ReadWriteMutexAPI
#endif
#include "ReadWriteMutex.h"
#include "ProcessReadWriteMutex.h"
#include "FileReadWriteMutex.h"
#include "ReadLock.h"
#include "WriteLock.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ReadWriteMutexd.lib")
#else
#pragma comment(lib,"ReadWriteMutex.lib")
#endif
#endif