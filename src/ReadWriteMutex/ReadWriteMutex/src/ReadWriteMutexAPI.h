#pragma once
#define _ReadWriteMutexAPI
#include "ReadWriteMutex.h"
#include "ProcessReadWriteMutex.h"
#include "ReadLock.h"
#include "WriteLock.h"

#ifdef _DEBUG
#pragma comment(lib,"ReadWriteMutexd.lib")
#else
#pragma comment(lib,"ReadWriteMutex.lib")
#endif