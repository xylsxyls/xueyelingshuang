#pragma once

#ifdef _MulPointAPI
	#define MulPointAPI _declspec(dllimport)
#else
	#define MulPointAPI _declspec(dllexport)
#endif
