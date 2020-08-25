#pragma once
#ifdef _WIN32
#define _RapidJsonAPI
#endif
#include "RapidJson.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"RapidJsond.lib")
#else
#pragma comment(lib,"RapidJson.lib")
#endif
#endif