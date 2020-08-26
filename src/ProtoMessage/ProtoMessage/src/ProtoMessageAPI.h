#pragma once
#ifdef _WIN32
#define _ProtoMessageAPI
#endif
#include "ProtoMessage.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ProtoMessaged.lib")
#else
#pragma comment(lib,"ProtoMessage.lib")
#endif
#endif