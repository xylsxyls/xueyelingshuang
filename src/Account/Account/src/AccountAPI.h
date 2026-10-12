#pragma once
#ifdef _WIN32
#define _AccountAPI
#endif
#include "Account.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Accountd.lib")
#else
#pragma comment(lib,"Account.lib")
#endif
#endif