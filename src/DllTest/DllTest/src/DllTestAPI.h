#pragma once
#define _DllTestAPI
#include "DllTest.h"

#ifdef _DEBUG
#pragma comment(lib,"DllTestd.lib")
#else
#pragma comment(lib,"DllTest.lib")
#endif