#pragma once

#ifdef _..proj..API
	#define ..proj..API _declspec(dllimport)
#else
	#define ..proj..API _declspec(dllexport)
#endif
