#pragma once

#ifdef _QssStringAPI
#define QssStringAPI _declspec(dllimport)
#else
#define QssStringAPI _declspec(dllexport)
#endif