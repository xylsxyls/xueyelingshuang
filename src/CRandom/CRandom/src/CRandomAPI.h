#pragma once
#define _CRandomAPI
#include "CRandom.h"

#ifdef _DEBUG
#pragma comment(lib,"CRandomd.lib")
#else
#pragma comment(lib,"CRandom.lib")
#endif