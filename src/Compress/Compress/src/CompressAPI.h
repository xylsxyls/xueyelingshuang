#pragma once
#define _CompressAPI
#include "Compress.h"

#ifdef _DEBUG
#pragma comment(lib,"Compressd.lib")
#else
#pragma comment(lib,"Compress.lib")
#endif