#pragma once
#ifdef _WIN32
#define _BigNumberBaseAPI
#endif
#include "BigNumberBase.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"BigNumberBased.lib")
#else
#pragma comment(lib,"BigNumberBase.lib")
#endif
#endif