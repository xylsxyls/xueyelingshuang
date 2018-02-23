#pragma once

#ifdef _ScreenScriptAPI
#define ScreenScriptAPI _declspec(dllimport)
#else
#define ScreenScriptAPI _declspec(dllexport)
#endif