#pragma once
#ifdef _WIN32
#define _NetInterfaceAPI
#endif
#include "NetInterface.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"NetInterfaced.lib")
#else
#pragma comment(lib,"NetInterface.lib")
#endif
#endif