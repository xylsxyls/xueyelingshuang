#pragma once
#ifdef _WIN32
#define _CtxtAPI
#endif
#include "Ctxt.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"Ctxtd.lib")
#else
#pragma comment(lib,"Ctxt.lib")
#endif
#endif