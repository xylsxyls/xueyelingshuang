#pragma once

#ifdef _QtControlsAPI
#define QtControlsAPI _declspec(dllexport)
#else
#define QtControlsAPI _declspec(dllimport)
#endif