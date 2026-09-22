#pragma once
#ifdef _WIN32
#define _SplitViewerCoreAPI
#endif
#include "SplitViewerCore.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"SplitViewerCored.lib")
#else
#pragma comment(lib,"SplitViewerCore.lib")
#endif
#endif