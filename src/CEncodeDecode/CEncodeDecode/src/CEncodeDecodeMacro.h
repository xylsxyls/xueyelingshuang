#pragma once

#ifdef _CEncodeDecodeAPI
#define CEncodeDecodeAPI _declspec(dllimport)
#else
#define CEncodeDecodeAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CEncodeDecodeAPI
#define CEncodeDecodeAPI 
#endif