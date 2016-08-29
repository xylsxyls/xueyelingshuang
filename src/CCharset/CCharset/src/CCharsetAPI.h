#pragma once
#define _CCharsetAPI
#include "CCharset.h"

#ifdef _DEBUG
#pragma comment(lib,"CCharsetd.lib")
#else
#pragma comment(lib,"CCharset.lib")
#endif