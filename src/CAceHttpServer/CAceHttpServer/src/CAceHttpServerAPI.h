#pragma once
#define _CAceHttpServerAPI
#include "CAceHttpServer.h"

#ifdef NDEBUG
#pragma comment(lib,"CAceHttpServer.lib")
#else
#pragma comment(lib,"CAceHttpServerd.lib")
#endif