#pragma once

#ifdef _LockFreeQueueAPI
#define LockFreeQueueAPI _declspec(dllimport)
#else
#define LockFreeQueueAPI _declspec(dllexport)
#endif