#pragma once
#define _BigNumberAPI
#include "BigNumber.h"

#ifdef _DEBUG
#pragma comment(lib,"BigNumberd.lib")
#else
#pragma comment(lib,"BigNumber.lib")
#endif