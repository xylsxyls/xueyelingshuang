#pragma once

#ifdef _ThreadWindowAPI
#define ThreadWindowAPI _declspec(dllimport)
#else
#define ThreadWindowAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ThreadWindowAPI
#define ThreadWindowAPI 
#endif