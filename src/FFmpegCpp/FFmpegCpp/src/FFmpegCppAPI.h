#pragma once
#ifdef _WIN32
#define _FFmpegCppAPI
#endif

#include "FFmpegCppModels.h"
#include "FFmpegCpp.h"
#include "FFmpegCppHelper.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"FFmpegCppd.lib")
#else
#pragma comment(lib,"FFmpegCpp.lib")
#endif
#endif