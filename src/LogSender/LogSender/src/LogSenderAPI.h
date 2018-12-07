#pragma once
#define _LogSenderAPI
#include "LogSender.h"

#ifdef _DEBUG
#pragma comment(lib,"LogSenderd.lib")
#else
#pragma comment(lib,"LogSender.lib")
#endif