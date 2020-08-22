#pragma once
#ifdef _WIN32
#define _BigNumberAPI
#endif
#include "BigNumber.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"BigNumberd.lib")
#else
#pragma comment(lib,"BigNumber.lib")
#endif
#endif