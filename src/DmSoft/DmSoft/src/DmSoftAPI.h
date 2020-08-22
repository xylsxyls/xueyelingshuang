#pragma once
#ifdef _WIN32
#define _DmSoftAPI
#endif
#include "DmSoft.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"DmSoftd.lib")
#else
#pragma comment(lib,"DmSoft.lib")
#endif
#endif