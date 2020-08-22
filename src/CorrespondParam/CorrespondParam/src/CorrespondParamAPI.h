#pragma once
#ifdef _WIN32
#define _CorrespondParamAPI
#endif
#include "CorrespondParam.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CorrespondParamd.lib")
#else
#pragma comment(lib,"CorrespondParam.lib")
#endif
#endif