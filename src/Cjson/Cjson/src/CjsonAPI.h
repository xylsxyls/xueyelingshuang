#pragma once
#define _CjsonAPI
#include "Cjson.h"

#ifdef NDEBUG
#pragma comment(lib,"Cjson.lib")
#else
#pragma comment(lib,"Cjsond.lib")
#endif