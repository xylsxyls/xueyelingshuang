#pragma once
#ifdef _WIN32
#define _ScreenScriptAPI
#endif
#include "ScreenScript.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ScreenScriptd.lib")
#else
#pragma comment(lib,"ScreenScript.lib")
#endif
#endif