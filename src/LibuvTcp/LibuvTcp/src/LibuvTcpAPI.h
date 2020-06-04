#pragma once
#define _LibuvTcpAPI
#include "LibuvTcp.h"

#ifdef _DEBUG
#pragma comment(lib,"LibuvTcpd.lib")
#else
#pragma comment(lib,"LibuvTcp.lib")
#endif