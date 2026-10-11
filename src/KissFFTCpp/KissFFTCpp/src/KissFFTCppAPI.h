#pragma once
#ifdef _WIN32
#define _KissFFTCppAPI
#endif
#include "KissFFTCpp.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"KissFFTCppd.lib")
#else
#pragma comment(lib,"KissFFTCpp.lib")
#endif
#endif