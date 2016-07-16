#pragma once
#define _CStringManagerAPI
#include "CStringManager.h"

#ifdef NDEBUG
#pragma comment(lib,"CStringManager.lib")
#else
#pragma comment(lib,"CStringManagerd.lib")
#endif