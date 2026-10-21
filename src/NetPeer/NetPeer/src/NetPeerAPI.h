#pragma once
#ifdef _WIN32
#define _NetPeerAPI
#endif
#include "NetPeer.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetPeerd.lib")
#else
#pragma comment(lib,"NetPeer.lib")
#endif
#endif