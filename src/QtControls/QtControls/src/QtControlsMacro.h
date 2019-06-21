#pragma once

#ifdef _QtControlsAPI
#define QtControlsAPI _declspec(dllexport)
#else
#define QtControlsAPI _declspec(dllimport)
#endif

#if defined(STATIC_LIB)
#undef QtControlsAPI
#define QtControlsAPI 
#endif