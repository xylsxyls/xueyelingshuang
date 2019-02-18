#pragma once

#ifdef _LogSenderAPI
#define LogSenderAPI _declspec(dllimport)
#else
#define LogSenderAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef LogSenderAPI
#define LogSenderAPI 
#endif