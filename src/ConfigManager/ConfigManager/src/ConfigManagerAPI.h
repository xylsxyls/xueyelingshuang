#pragma once
#ifdef _WIN32
#define _ConfigManagerAPI
#endif
#include "ConfigManager.h"
#include "UserConfigManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ConfigManagerd.lib")
#else
#pragma comment(lib,"ConfigManager.lib")
#endif
#endif