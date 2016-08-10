#pragma once
#define _CAceTCPClientAPI
#include "CAceTCPClient.h"

#ifdef _DEBUG
#pragma comment(lib,"CAceTCPClientd.lib")
#else
#pragma comment(lib,"CAceTCPClient.lib")
#endif