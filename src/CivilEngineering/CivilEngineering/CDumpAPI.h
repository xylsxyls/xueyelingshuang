#pragma once
#define _CDumpAPI
#include "CDump.h"

#ifdef NDEBUG
#pragma comment(lib,"CDump.lib")
#else
#pragma comment(lib,"CDumpd.lib")
#endif