#pragma once

#ifdef _LockFreeQueueAPI
#define LockFreeQueueAPI _declspec(dllimport)
#else
#define LockFreeQueueAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef LockFreeQueueAPI
#define LockFreeQueueAPI 
#endif