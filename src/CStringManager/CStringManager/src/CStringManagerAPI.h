#pragma once
#define _CStringManagerAPI
#include "CStringManager.h"

#ifdef _DEBUG
#pragma comment(lib,"CStringManagerd.lib")
#else
#pragma comment(lib,"CStringManager.lib")
#endif