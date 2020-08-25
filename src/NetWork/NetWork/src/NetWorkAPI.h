#pragma once
#ifdef _WIN32
#define _NetWorkAPI
#endif
#include "NetClient.h"
#include "NetServer.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetWorkd.lib")
#else
#pragma comment(lib,"NetWork.lib")
#endif
#endif