#pragma once
#ifdef _WIN32
#define _NetSenderAPI
#endif
#include "NetSender.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetSenderd.lib")
#else
#pragma comment(lib,"NetSender.lib")
#endif
#endif