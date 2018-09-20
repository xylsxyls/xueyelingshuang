#pragma once
#define _SqlStringAPI
#include "SqlString.h"

#ifdef _DEBUG
#pragma comment(lib,"SqlStringd.lib")
#else
#pragma comment(lib,"SqlString.lib")
#endif