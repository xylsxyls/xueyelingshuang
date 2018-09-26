#pragma once
#define _ConfigManagerAPI
#include "ConfigManager.h"
#include "UserConfigManager.h"

#ifdef _DEBUG
#pragma comment(lib,"ConfigManagerd.lib")
#else
#pragma comment(lib,"ConfigManager.lib")
#endif