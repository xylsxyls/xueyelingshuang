#pragma once
#define _LogManagerAPI
#include "LogManager.h"

#ifdef _DEBUG
#pragma comment(lib,"LogManagerd.lib")
#else
#pragma comment(lib,"LogManager.lib")
#endif