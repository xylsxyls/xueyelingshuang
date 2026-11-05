#pragma once

#ifdef _MSC_VER
    #ifdef _LockFreeHashMapAPI
        #define LockFreeHashMapAPI _declspec(dllimport)
    #else
        #define LockFreeHashMapAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LockFreeHashMapAPI __attribute__ ((visibility("default")))
    #else
        #define LockFreeHashMapAPI
    #endif
#endif

#if defined(STATIC_LIB)
    #undef LockFreeHashMapAPI
    #define LockFreeHashMapAPI 
#endif

// 仅给无异常的短读取路径提供编译器提示，六端使用相同算法与公开接口
#if defined(_MSC_VER) && defined(NDEBUG)
#define LOCKFREEHASHMAP_FORCE_INLINE __forceinline
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE__)
#define LOCKFREEHASHMAP_FORCE_INLINE inline __attribute__((always_inline))
#else
#define LOCKFREEHASHMAP_FORCE_INLINE inline
#endif