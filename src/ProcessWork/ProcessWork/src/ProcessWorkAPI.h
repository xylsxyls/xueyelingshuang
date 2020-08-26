#pragma once
#ifdef _WIN32
#define _ProcessWorkAPI
#endif
#include "ProcessWork.h"
#include "ProcessReceiveCallback.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ProcessWorkd.lib")
#else
#pragma comment(lib,"ProcessWork.lib")
#endif
#endif