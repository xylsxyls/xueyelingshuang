#pragma once
#ifdef _WIN32
#define _DllTestAPI
#endif
#include "DllTest.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"DllTestd.lib")
#else
#pragma comment(lib,"DllTest.lib")
#endif
#endif