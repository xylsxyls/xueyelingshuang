#pragma once
#define _RapidJsonAPI
#include "RapidJson.h"

#ifdef _DEBUG
#pragma comment(lib,"RapidJsond.lib")
#else
#pragma comment(lib,"RapidJson.lib")
#endif