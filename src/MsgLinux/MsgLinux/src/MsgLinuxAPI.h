#pragma once
#ifdef _WIN32
#define _MsgLinuxAPI
#endif
#include "MsgLinux.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"MsgLinuxd.lib")
#else
#pragma comment(lib,"MsgLinux.lib")
#endif
#endif