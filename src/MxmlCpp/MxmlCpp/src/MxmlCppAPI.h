#pragma once
#ifdef _WIN32
#define _MxmlCppAPI
#endif
#include "MxmlCpp.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"MxmlCppd.lib")
#else
#pragma comment(lib,"MxmlCpp.lib")
#endif
#endif