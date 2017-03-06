#pragma once
#define _DmSoftAPI
#include "DmSoft.h"

#ifdef _DEBUG
#pragma comment(lib,"DmSoftd.lib")
#else
#pragma comment(lib,"DmSoft.lib")
#endif