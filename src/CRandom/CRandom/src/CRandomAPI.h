#pragma once
#ifdef _WIN32
#define _CRandomAPI
#endif
#include "CRandom.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CRandomd.lib")
#else
#pragma comment(lib,"CRandom.lib")
#endif
#endif