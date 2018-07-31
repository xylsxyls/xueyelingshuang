#pragma once

#ifdef _NetWorkAPI
#define NetWorkAPI _declspec(dllimport)
#else
#define NetWorkAPI _declspec(dllexport)
#endif