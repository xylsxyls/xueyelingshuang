#pragma once

#ifdef _CBrainAPI
	#define CBrainAPI _declspec(dllimport)
#else
	#define CBrainAPI _declspec(dllexport)
#endif
