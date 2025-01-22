#pragma once
#ifdef _WIN32
#define _SharedMemoryAPI
#endif
#include "SharedMemory.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"SharedMemoryd.lib")
#else
#pragma comment(lib,"SharedMemory.lib")
#endif
#endif