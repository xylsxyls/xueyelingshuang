#pragma once
#ifdef _WIN32
#define _CTaskThreadManagerAPI
#endif
#include "CTaskThreadManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CTaskThreadManagerd.lib")
#else
#pragma comment(lib,"CTaskThreadManager.lib")
#endif
#endif