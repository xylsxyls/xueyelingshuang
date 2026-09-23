#pragma once

#ifdef _WIN32
#define _PdfReaderCoreAPI
#endif

#include "PdfReaderCoreMacro.h"

#ifdef PDFREADERCORE_USE_C_API
#include "PdfReaderCoreC.h"
#else
#include "PdfReaderCore.h"
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"PdfReaderCored.lib")
#else
#pragma comment(lib,"PdfReaderCore.lib")
#endif
#endif