#pragma once
#ifdef _WIN32
#define _QssStringAPI
#endif
#include "QssString.h"
#include "QssHelper.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"QssStringd.lib")
#else
#pragma comment(lib,"QssString.lib")
#endif
#endif