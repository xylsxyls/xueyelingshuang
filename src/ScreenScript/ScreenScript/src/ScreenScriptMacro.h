#pragma once

#ifdef _ScreenScriptAPI
#define ScreenScriptAPI _declspec(dllimport)
#else
#define ScreenScriptAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ScreenScriptAPI
#define ScreenScriptAPI 
#endif