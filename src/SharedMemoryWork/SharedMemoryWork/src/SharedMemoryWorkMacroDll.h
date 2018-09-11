#pragma once

#ifdef _SharedMemoryWorkAPI
#define SharedMemoryWorkAPI _declspec(dllimport)
#else
#define SharedMemoryWorkAPI _declspec(dllexport)
#endif