#pragma once
#define _ProcessWorkAPI
#include "ProcessWork.h"
#include "ProcessReceiveCallback.h"

#ifdef _DEBUG
#pragma comment(lib,"ProcessWorkd.lib")
#else
#pragma comment(lib,"ProcessWork.lib")
#endif