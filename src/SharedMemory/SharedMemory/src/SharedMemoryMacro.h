#pragma once

#ifdef _MSC_VER
    #ifdef _SharedMemoryAPI
        #define SharedMemoryAPI _declspec(dllimport)
    #else
        #define SharedMemoryAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define SharedMemoryAPI __attribute__ ((visibility("default")))
    #else
	    #define SharedMemoryAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef SharedMemoryAPI
    #define SharedMemoryAPI 
#endif