#pragma once
#ifdef _WIN32
#define _IntDateTimeAPI
#endif
#include "IntDateTime.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"IntDateTimed.lib")
#else
#pragma comment(lib,"IntDateTime.lib")
#endif
#endif