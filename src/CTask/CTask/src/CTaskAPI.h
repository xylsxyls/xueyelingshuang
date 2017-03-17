#pragma once
#define _CTaskAPI
#include "CTask.h"

#ifdef _DEBUG
#pragma comment(lib,"CTaskd.lib")
#else
#pragma comment(lib,"CTask.lib")
#endif