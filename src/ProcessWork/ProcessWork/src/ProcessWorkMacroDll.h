#pragma once

#ifdef _ProcessWorkAPI
#define ProcessWorkAPI _declspec(dllimport)
#else
#define ProcessWorkAPI _declspec(dllexport)
#endif