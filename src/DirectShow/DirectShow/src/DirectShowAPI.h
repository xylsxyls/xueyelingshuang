#pragma once
#ifdef _WIN32
#define _DirectShowAPI
#endif
#include "DirectShow.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"DirectShowd.lib")
#else
#pragma comment(lib,"DirectShow.lib")
#endif
#endif