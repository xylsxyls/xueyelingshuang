#pragma once
#ifdef _WIN32
#define _CGetPathAPI
#endif
#include "CGetPath.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CGetPathd.lib")
#else
#pragma comment(lib,"CGetPath.lib")
#endif
#endif