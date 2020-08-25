#pragma once
#ifdef _WIN32
#define _LibuvTcpAPI
#endif
#include "LibuvTcp.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"LibuvTcpd.lib")
#else
#pragma comment(lib,"LibuvTcp.lib")
#endif
#endif