#pragma once

#ifdef _CompressAPI
#define CompressAPI _declspec(dllimport)
#else
#define CompressAPI _declspec(dllexport)
#endif