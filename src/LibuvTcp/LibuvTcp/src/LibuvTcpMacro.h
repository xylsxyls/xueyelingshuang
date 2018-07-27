#pragma once

#ifdef _LibuvTcpAPI
#define LibuvTcpAPI _declspec(dllimport)
#else
#define LibuvTcpAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef LibuvTcpAPI
#define LibuvTcpAPI 
#endif