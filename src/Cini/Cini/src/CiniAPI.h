#pragma once
#ifdef _WIN32
#define _CiniAPI
#endif
#include "Cini.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Cinid.lib")
#else
#pragma comment(lib,"Cini.lib")
#endif
#endif