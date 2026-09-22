#pragma once
#ifdef _WIN32
#define _PdfReaderCoreAPI
#endif
#include "PdfReaderCore.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"PdfReaderCored.lib")
#else
#pragma comment(lib,"PdfReaderCore.lib")
#endif
#endif