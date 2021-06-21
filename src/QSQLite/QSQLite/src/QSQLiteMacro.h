#pragma once

#ifdef _MSC_VER
    #ifdef _QSQLiteAPI
        #define QSQLiteAPI _declspec(dllimport)
    #else
        #define QSQLiteAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define QSQLiteAPI __attribute__ ((visibility("default")))
    #else
        #define QSQLiteAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef QSQLiteAPI
    #define QSQLiteAPI 
#endif