#pragma once
#define _QtClientAPI
#include "QtClient.h"

#ifdef _DEBUG
#pragma comment(lib,"QtClientd.lib")
#else
#pragma comment(lib,"QtClient.lib")
#endif