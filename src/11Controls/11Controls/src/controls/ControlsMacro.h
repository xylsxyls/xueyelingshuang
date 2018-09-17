#pragma once

#ifdef _ControlsAPI
#define ControlsAPI _declspec(dllexport)
#else
#define ControlsAPI _declspec(dllimport)
#endif

#ifdef _ControlsAPI
#elif _DEBUG
#pragma comment(lib, "11Controlsd.lib")
#else
#pragma comment(lib, "11Controls.lib")
#endif