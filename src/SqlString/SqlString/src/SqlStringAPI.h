#pragma once
#ifdef _WIN32
#define _SqlStringAPI
#endif
#include "SqlString.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"SqlStringd.lib")
#else
#pragma comment(lib,"SqlString.lib")
#endif
#endif