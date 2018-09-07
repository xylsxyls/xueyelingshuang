#pragma once
#define _SharedMemoryAPI
#include "SharedMemory.h"

#ifdef _DEBUG
#pragma comment(lib,"SharedMemoryd.lib")
#else
#pragma comment(lib,"SharedMemory.lib")
#endif