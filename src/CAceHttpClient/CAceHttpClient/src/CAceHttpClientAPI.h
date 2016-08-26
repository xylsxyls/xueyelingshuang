#pragma once
#define _CAceHttpClientAPI
#include "CAceHttpClient.h"

#ifdef NDEBUG
#pragma comment(lib,"CAceHttpClient.lib")
#else
#pragma comment(lib,"CAceHttpClientd.lib")
#endif