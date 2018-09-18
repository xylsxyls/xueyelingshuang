#pragma once

#ifdef _QtDllTestAPI
#define QtDllTestAPI _declspec(dllimport)
#else
#define QtDllTestAPI _declspec(dllexport)
#endif