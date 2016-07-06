#pragma once
#define _CGetPathAPI
#include "CGetPath.h"

#ifdef NDEBUG
#pragma comment(lib,"CGetPath.lib")
#else
#pragma comment(lib,"CGetPathd.lib")
#endif