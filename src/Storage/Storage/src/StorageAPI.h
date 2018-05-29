#pragma once
#define _StorageAPI
#include "Storage.h"

#ifdef _DEBUG
#pragma comment(lib,"Storaged.lib")
#else
#pragma comment(lib,"Storage.lib")
#endif