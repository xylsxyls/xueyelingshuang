#pragma once

#ifdef _FastNumberAPI
#define FastNumberAPI _declspec(dllimport)
#else
#define FastNumberAPI _declspec(dllexport)
#endif