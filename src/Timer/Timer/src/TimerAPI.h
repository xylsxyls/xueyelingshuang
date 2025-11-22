#pragma once
#ifdef _WIN32
#define _TimerAPI
#endif
#include "Timer.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Timerd.lib")
#else
#pragma comment(lib,"Timer.lib")
#endif
#endif