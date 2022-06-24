#pragma once
#ifdef _WIN32
#define _LevelDBManagerAPI
#endif
#include "LevelDBManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LevelDBManagerd.lib")
#else
#pragma comment(lib,"LevelDBManager.lib")
#endif
#endif