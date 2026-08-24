#pragma once

#ifdef _MSC_VER
    #ifdef _FFmpegCppAPI
        #define FFmpegCppAPI _declspec(dllimport)
    #else
        #define FFmpegCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define FFmpegCppAPI __attribute__ ((visibility("default")))
    #else
        #define FFmpegCppAPI
    #endif
#endif