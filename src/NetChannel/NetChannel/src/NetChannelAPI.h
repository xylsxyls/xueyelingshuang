#pragma once
#ifdef _WIN32
#define _NetChannelAPI
#endif
#include "NetChannel.h"
#include "NetChannelCredit.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetChanneld.lib")
#else
#pragma comment(lib,"NetChannel.lib")
#endif
#endif