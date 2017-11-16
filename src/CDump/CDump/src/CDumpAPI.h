#pragma once
#define _CDumpAPI
#include "CDump.h"

#ifdef _DEBUG
#pragma comment(lib,"CDumpd.lib")
#else
#pragma comment(lib,"CDump.lib")
#endif