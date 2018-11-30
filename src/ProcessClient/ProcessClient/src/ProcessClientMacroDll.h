#pragma once

#ifdef _ProcessClientAPI
#define ProcessClientAPI _declspec(dllimport)
#else
#define ProcessClientAPI _declspec(dllexport)
#endif