#pragma once

#ifdef _CtxtAPI
	#define CtxtAPI _declspec(dllimport)
#else
	#define CtxtAPI _declspec(dllexport)
#endif
