#pragma once
#ifdef _WIN32
#define _CCharsetAPI
#endif
#include "CCharset.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CCharsetd.lib")
#else
#pragma comment(lib,"CCharset.lib")
#endif
#endif