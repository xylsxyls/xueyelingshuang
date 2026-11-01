#pragma once

#ifdef _MSC_VER
    #ifdef _JsonCppAPI
        #define JsonCppAPI _declspec(dllimport)
    #else
        #define JsonCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define JsonCppAPI __attribute__ ((visibility("default")))
    #else
        #define JsonCppAPI
    #endif
#endif

// 上游公开类型使用JSON_API，保持与源码和发布头的导入导出定义一致
#define JSON_API JsonCppAPI