#pragma once

#ifdef _SemaphoreAPI
#define SemaphoreAPI _declspec(dllimport)
#else
#define SemaphoreAPI _declspec(dllexport)
#endif