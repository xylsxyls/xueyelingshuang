#pragma once
#ifdef _WIN32
#define _ThreadWindowAPI
#endif
#include "ThreadWindow.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ThreadWindowd.lib")
#else
#pragma comment(lib,"ThreadWindow.lib")
#endif
#endif