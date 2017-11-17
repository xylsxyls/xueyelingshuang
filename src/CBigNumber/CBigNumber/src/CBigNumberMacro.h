#pragma once

#ifdef _CBigNumberAPI
#define CBigNumberAPI _declspec(dllimport)
#else
#define CBigNumberAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef CBigNumberAPI
#define CBigNumberAPI 
#endif