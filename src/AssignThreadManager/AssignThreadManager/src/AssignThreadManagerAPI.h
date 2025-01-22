#pragma once
#ifdef _WIN32
#define _AssignThreadManagerAPI
#endif
#include "AssignThreadManager.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"AssignThreadManagerd.lib")
#else
#pragma comment(lib,"AssignThreadManager.lib")
#endif
#endif