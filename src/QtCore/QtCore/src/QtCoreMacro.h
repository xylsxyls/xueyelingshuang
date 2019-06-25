#pragma once

#ifdef _QtCoreAPI
#define QtCoreAPI _declspec(dllimport)
#else
#define QtCoreAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef QtCoreAPI
#define QtCoreAPI 
#endif