#pragma once
#ifdef _WIN32
#define _CScreenAPI
#endif
#include "CScreen.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CScreend.lib")
#else
#pragma comment(lib,"CScreen.lib")
#endif
#endif