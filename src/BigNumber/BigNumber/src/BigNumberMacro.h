#pragma once

#ifdef _BigNumberAPI
#define BigNumberAPI _declspec(dllimport)
#else
#define BigNumberAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef BigNumberAPI
#define BigNumberAPI 
#endif