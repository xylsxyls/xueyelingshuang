#pragma once
#ifdef _WIN32
#define _ColorAPI
#endif
#include "Color.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Colord.lib")
#else
#pragma comment(lib,"Color.lib")
#endif
#endif