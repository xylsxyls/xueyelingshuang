#pragma once

#ifdef STATIC_LIB
#define QtControlsAPI 
#else
#define QtControlsAPI _declspec(dllexport)
#endif