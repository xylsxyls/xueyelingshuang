#pragma once

#ifdef _IntDateTimeAPI
	#define IntDateTimeAPI _declspec(dllimport)
#else
	#define IntDateTimeAPI _declspec(dllexport)
#endif
