#pragma once
#ifdef _WIN32
#define _RectAPI
#endif
#include "Rect.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Rectd.lib")
#else
#pragma comment(lib,"Rect.lib")
#endif
#endif