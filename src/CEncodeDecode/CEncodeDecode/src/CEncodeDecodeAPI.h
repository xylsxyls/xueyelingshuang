#pragma once
#ifdef _WIN32
#define _CEncodeDecodeAPI
#endif
#include "CEncodeDecode.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CEncodeDecoded.lib")
#else
#pragma comment(lib,"CEncodeDecode.lib")
#endif
#endif