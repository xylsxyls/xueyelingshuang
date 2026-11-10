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

// 发布时保留六十四位原子的显式八字节对齐，防止 Win32 栈对象出现跨缓存行原子操作。
#if defined(_MSC_VER)
#define LOCKFREEHASHMAP_ALIGN_64 __declspec(align(8))
#else
#define LOCKFREEHASHMAP_ALIGN_64 alignas(8)
#endif

// 发布时本文件替换主宏头，必须同时保留模板读取路径使用的编译器提示。
#if defined(_MSC_VER) && defined(NDEBUG)
#define LOCKFREEHASHMAP_FORCE_INLINE __forceinline
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE__)
#define LOCKFREEHASHMAP_FORCE_INLINE inline __attribute__((always_inline))
#else
#define LOCKFREEHASHMAP_FORCE_INLINE inline
#endif