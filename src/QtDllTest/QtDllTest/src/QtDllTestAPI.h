#pragma once
#define _QtDllTestAPI
#include "QtDllTest.h"

#ifdef _DEBUG
#pragma comment(lib,"QtDllTestd.lib")
#else
#pragma comment(lib,"QtDllTest.lib")
#endif