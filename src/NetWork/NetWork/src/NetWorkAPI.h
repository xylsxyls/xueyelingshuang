#pragma once
#define _NetWorkAPI
#include "NetClient.h"
#include "NetServer.h"

#ifdef _DEBUG
#pragma comment(lib,"NetWorkd.lib")
#else
#pragma comment(lib,"NetWork.lib")
#endif