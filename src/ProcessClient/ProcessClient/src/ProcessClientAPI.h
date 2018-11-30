#pragma once
#define _ProcessClientAPI
#include "ProcessClient.h"
#include "ReceiveCallback.h"

#ifdef _DEBUG
#pragma comment(lib,"ProcessClientd.lib")
#else
#pragma comment(lib,"ProcessClient.lib")
#endif