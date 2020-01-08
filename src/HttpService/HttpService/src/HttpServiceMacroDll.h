#pragma once

#ifdef _HttpServiceAPI
#define HttpServiceAPI _declspec(dllimport)
#else
#define HttpServiceAPI _declspec(dllexport)
#endif