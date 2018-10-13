#pragma once
#define _ProcessWorkAPI
#include "ProcessClient.h"
#include "ProcessServer.h"
#include "ServerReceiveCallback.h"

#ifdef _DEBUG
#pragma comment(lib,"ProcessWorkd.lib")
#else
#pragma comment(lib,"ProcessWork.lib")
#endif