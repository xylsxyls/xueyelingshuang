#pragma once
#define _CCheckConnectAPI
#include "CCheckConnect.h"

#ifdef NDEBUG
#pragma comment(lib,"CCheckConnect.lib")
#else
#pragma comment(lib,"CCheckConnectd.lib")
#endif