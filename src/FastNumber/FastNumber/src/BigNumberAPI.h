#pragma once
#define _FastNumberAPI
#include "FastNumber.h"

typedef FastNumber BigNumber;

#ifdef _DEBUG
#pragma comment(lib,"FastNumberd.lib")
#else
#pragma comment(lib,"FastNumber.lib")
#endif