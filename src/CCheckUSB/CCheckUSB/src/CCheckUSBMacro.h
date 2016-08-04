#pragma once

#ifdef _CCheckUSBAPI
	#define CCheckUSBAPI _declspec(dllimport)
#else
	#define CCheckUSBAPI _declspec(dllexport)
#endif
