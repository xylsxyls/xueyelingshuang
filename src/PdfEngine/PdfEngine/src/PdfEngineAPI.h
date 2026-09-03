#pragma once
#ifdef _WIN32
#define _PdfEngineAPI
#endif
#include "PdfEngine.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"PdfEngined.lib")
#else
#pragma comment(lib,"PdfEngine.lib")
#endif
#endif