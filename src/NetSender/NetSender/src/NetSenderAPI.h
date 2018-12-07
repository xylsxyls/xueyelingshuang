#pragma once
#define _NetSenderAPI
#include "NetSender.h"

#ifdef _DEBUG
#pragma comment(lib,"NetSenderd.lib")
#else
#pragma comment(lib,"NetSender.lib")
#endif