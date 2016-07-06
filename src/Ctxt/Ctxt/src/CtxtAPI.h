#pragma once
#define _CtxtAPI
#include "Ctxt.h"

#ifdef NDEBUG
#pragma comment(lib,"Ctxt.lib")
#else
#pragma comment(lib,"Ctxtd.lib")
#endif