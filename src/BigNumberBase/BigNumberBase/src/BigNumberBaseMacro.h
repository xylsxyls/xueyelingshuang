#pragma once

#ifdef _BigNumberBaseAPI
#define BigNumberBaseAPI _declspec(dllimport)
#else
#define BigNumberBaseAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef BigNumberBaseAPI
#define BigNumberBaseAPI 
#endif