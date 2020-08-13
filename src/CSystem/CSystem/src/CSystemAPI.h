#pragma once
#ifdef _WIN32
#define _CSystemAPI
#endif
#include "CSystem.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CSystemd.lib")
#else
#pragma comment(lib,"CSystem.lib")
#endif
#endif