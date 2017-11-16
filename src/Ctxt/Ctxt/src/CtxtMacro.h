#pragma once

#ifdef _CtxtAPI
#define CtxtAPI _declspec(dllimport)
#else
#define CtxtAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CtxtAPI
#define CtxtAPI 
#endif