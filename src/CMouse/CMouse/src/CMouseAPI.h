#pragma once
#ifdef _WIN32
#define _CMouseAPI
#endif
#include "CMouse.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CMoused.lib")
#else
#pragma comment(lib,"CMouse.lib")
#endif
#endif