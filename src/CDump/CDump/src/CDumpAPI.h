#pragma once
#ifdef _WIN32
#define _CDumpAPI
#endif
#include "CDump.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CDumpd.lib")
#else
#pragma comment(lib,"CDump.lib")
#endif
#endif