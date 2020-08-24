#pragma once
#ifdef _WIN32
#define _SemaphoreAPI
#endif
#include "Semaphore.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Semaphored.lib")
#else
#pragma comment(lib,"Semaphore.lib")
#endif
#endif