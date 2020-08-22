#pragma once
#ifdef _WIN32
#define _CHookAPI
#endif
#include "CHook.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CHookd.lib")
#else
#pragma comment(lib,"CHook.lib")
#endif
#endif