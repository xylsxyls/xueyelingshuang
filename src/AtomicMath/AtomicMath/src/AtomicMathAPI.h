#pragma once
#ifdef _WIN32
#define _AtomicMathAPI
#endif
#include "AtomicMath.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"AtomicMathd.lib")
#else
#pragma comment(lib,"AtomicMath.lib")
#endif
#endif