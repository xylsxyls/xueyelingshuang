#pragma once

#ifdef _NetSenderAPI
#define NetSenderAPI _declspec(dllimport)
#else
#define NetSenderAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef NetSenderAPI
#define NetSenderAPI 
#endif