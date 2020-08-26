#pragma once

#ifdef _MSC_VER
    #ifdef _MysqlCppAPI
        #define MysqlCppAPI _declspec(dllimport)
    #else
        #define MysqlCppAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define MysqlCppAPI __attribute__ ((visibility("default")))
    #else
        #define MysqlCppAPI
    #endif
#endif