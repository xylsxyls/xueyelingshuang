#pragma once
#ifdef _WIN32
#define _CKeyboardConfigAPI
#endif
#include "CKeyboardConfig.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CKeyboardConfigd.lib")
#else
#pragma comment(lib,"CKeyboardConfig.lib")
#endif
#endif