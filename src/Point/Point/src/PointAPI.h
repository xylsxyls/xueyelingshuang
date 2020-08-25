#pragma once
#ifdef _WIN32
#define _PointAPI
#endif
#include "Point.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Pointd.lib")
#else
#pragma comment(lib,"Point.lib")
#endif
#endif