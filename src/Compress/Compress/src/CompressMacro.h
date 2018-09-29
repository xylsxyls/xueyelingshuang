#pragma once

#ifdef _CompressAPI
#define CompressAPI _declspec(dllimport)
#else
#define CompressAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CompressAPI
#define CompressAPI 
#endif