#pragma once
#define _SemaphoreAPI
#include "Semaphore.h"

#ifdef _DEBUG
#pragma comment(lib,"Semaphored.lib")
#else
#pragma comment(lib,"Semaphore.lib")
#endif