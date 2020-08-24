#pragma once
#ifdef _WIN32
#define _LogManagerAPI
#endif
#include "LogManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LogManagerd.lib")
#else
#pragma comment(lib,"LogManager.lib")
#endif
#endif