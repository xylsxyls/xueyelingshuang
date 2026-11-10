#pragma once
#ifdef _WIN32
#define _ProtoMessageManagerAPI
#endif
#include "ProtoMessageManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ProtoMessageManagerd.lib")
#else
#pragma comment(lib,"ProtoMessageManager.lib")
#endif
#endif