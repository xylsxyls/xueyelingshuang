#pragma once

#ifdef _CMysqlAPI
	#define API _declspec(dllimport)
#else
	#define API _declspec(dllexport)
#endif
