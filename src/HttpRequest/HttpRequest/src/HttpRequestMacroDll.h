#pragma once

#ifdef _HttpRequestAPI
#define HttpRequestAPI _declspec(dllimport)
#else
#define HttpRequestAPI _declspec(dllexport)
#endif