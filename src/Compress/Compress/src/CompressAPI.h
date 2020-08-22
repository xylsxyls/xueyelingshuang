#pragma once
#ifdef _WIN32
#define _CompressAPI
#endif
#include "Compress.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Compressd.lib")
#else
#pragma comment(lib,"Compress.lib")
#endif
#endif