#pragma once

#ifdef _ColorAPI
#define ColorAPI _declspec(dllimport)
#else
#define ColorAPI _declspec(dllexport)
#endif