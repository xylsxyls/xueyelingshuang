#pragma once

#ifdef _QtClientAPI
#define QtClientAPI _declspec(dllimport)
#else
#define QtClientAPI _declspec(dllexport)
#endif