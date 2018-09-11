#pragma once
#define _SharedMemoryWorkAPI
#include "SharedMemoryClient.h"
#include "SharedMemoryServer.h"
#include "ServerReceiveCallback.h"

#ifdef _DEBUG
#pragma comment(lib,"SharedMemoryWorkd.lib")
#else
#pragma comment(lib,"SharedMemoryWork.lib")
#endif