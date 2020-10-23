#pragma once

#ifdef _MSC_VER
    #define QtControlsAPI _declspec(dllimport)
    #ifdef _DEBUG
        #pragma comment(lib,"QtControlsd.lib")
    #else
        #pragma comment(lib,"QtControls.lib")
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define QtControlsAPI __attribute__ ((visibility("default")))
    #else
        #define QtControlsAPI
    #endif
#endif