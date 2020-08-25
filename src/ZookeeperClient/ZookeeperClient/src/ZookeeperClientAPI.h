#pragma once
#ifdef _WIN32
#define _ZookeeperClientAPI
#endif
#include "ZookeeperClient.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ZookeeperClientd.lib")
#else
#pragma comment(lib,"ZookeeperClient.lib")
#endif
#endif