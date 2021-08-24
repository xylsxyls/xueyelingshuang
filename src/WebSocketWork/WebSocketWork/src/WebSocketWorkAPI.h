#pragma once
#ifdef _WIN32
#define _WebSocketWorkAPI
#endif
#include "WebSocketClient.h"
#include "WebSocketServer.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"WebSocketWorkd.lib")
#else
#pragma comment(lib,"WebSocketWork.lib")
#endif
#endif