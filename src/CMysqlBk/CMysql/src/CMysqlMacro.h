#pragma once

#ifdef _CMysqlAPI
	#define CMysqlAPI _declspec(dllimport)
#else
	#define CMysqlAPI _declspec(dllexport)
#endif
