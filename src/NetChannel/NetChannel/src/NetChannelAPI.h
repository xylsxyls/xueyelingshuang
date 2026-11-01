#pragma once
#if defined(_WIN32) && !defined(NET_CHANNEL_BUILD)
#define _NetChannelAPI
#endif
#include "NetChannel.h"
#include "NetChannelCredit.h"

#if defined(_WIN32) && !defined(NET_CHANNEL_NO_AUTOLINK)
#if defined _DEBUG
#pragma comment(lib,"NetChanneld.lib")
#else
#pragma comment(lib,"NetChannel.lib")
#endif
#endif