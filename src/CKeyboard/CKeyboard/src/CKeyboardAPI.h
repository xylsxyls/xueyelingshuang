#pragma once
#ifdef _WIN32
#define _CKeyboardAPI
#endif
#include "CKeyboard.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CKeyboardd.lib")
#else
#pragma comment(lib,"CKeyboard.lib")
#endif
#endif