#pragma once
#ifdef _WIN32
#define _QtDllTestAPI
#endif
#include "QtDllTest.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"QtDllTestd.lib")
#else
#pragma comment(lib,"QtDllTest.lib")
#endif
#endif