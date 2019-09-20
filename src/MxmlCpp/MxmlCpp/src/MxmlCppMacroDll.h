#pragma once

#ifdef _MxmlCppAPI
#define MxmlCppAPI _declspec(dllimport)
#else
#define MxmlCppAPI _declspec(dllexport)
#endif