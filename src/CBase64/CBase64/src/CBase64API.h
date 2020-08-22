#pragma once
#ifdef _WIN32
#define _CBase64API
#endif
#include "CBase64.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CBase64d.lib")
#else
#pragma comment(lib,"CBase64.lib")
#endif
#endif