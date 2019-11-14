#pragma once
#define _ZookeeperClientAPI
#include "ZookeeperClient.h"

#ifdef _DEBUG
#pragma comment(lib,"ZookeeperClientd.lib")
#else
#pragma comment(lib,"ZookeeperClient.lib")
#endif