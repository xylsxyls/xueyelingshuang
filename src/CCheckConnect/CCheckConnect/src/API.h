#pragma once

#ifdef _CCheckConnectAPI
	#define CCheckConnectAPI _declspec(dllimport)
#else
	#define CCheckConnectAPI _declspec(dllexport)
#endif
