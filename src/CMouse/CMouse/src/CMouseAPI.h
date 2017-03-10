#pragma once
#define _CMouseAPI
#include "CMouse.h"

#ifdef _DEBUG
#pragma comment(lib,"CMoused.lib")
#else
#pragma comment(lib,"CMouse.lib")
#endif