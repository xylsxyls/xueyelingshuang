#pragma once

#ifdef _CDumpAPI
	#define CDumpAPI _declspec(dllimport)
#else
	#define CDumpAPI _declspec(dllexport)
#endif
