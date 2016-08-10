#pragma once
#define _CAceTCPServerAPI
#include "CAceTCPServer.h"

#ifdef NDEBUG
#pragma comment(lib,"CAceTCPServer.lib")
#else
#pragma comment(lib,"CAceTCPServerd.lib")
#endif