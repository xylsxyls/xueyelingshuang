#pragma once
#ifdef _WIN32
#define _CStringManagerAPI
#endif
#include "CStringManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CStringManagerd.lib")
#else
#pragma comment(lib,"CStringManager.lib")
#endif
#endif