#pragma once

#ifdef _SharedMemoryAPI
#define SharedMemoryAPI _declspec(dllimport)
#else
#define SharedMemoryAPI _declspec(dllexport)
#endif