#pragma once
#ifdef _WIN32
#define _RapidJsonCppAPI
#endif
#include "RapidJsonDocument.h"
#include "RapidJsonCpp.h"
#include "RapidJsonValue.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"RapidJsonCppd.lib")
#else
#pragma comment(lib,"RapidJsonCpp.lib")
#endif
#endif