#pragma once
#define _CjsonAPI
#include "Cjson.h"

#ifdef _DEBUG
#pragma comment(lib,"Cjsond.lib")
#else
#pragma comment(lib,"Cjson.lib")
#endif