#pragma once
#ifdef _WIN32
#define _FastNumberAPI
#endif
#include "FastNumber.h"

typedef FastNumber BigNumber;

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"FastNumberd.lib")
#else
#pragma comment(lib,"FastNumber.lib")
#endif
#endif