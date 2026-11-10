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

// v120 Win32 的默认栈布局不能保证六十四位原子字段的八字节地址对齐。
#if defined(_MSC_VER)
#define LOCKFREEHASHMAP_ALIGN_64 __declspec(align(8))
#else
#define LOCKFREEHASHMAP_ALIGN_64 alignas(8)
#endif

// 仅给无异常的短原子操作和读取路径提供编译器提示，六端使用相同算法与公开接口
#if defined(_MSC_VER) && defined(NDEBUG)
#define LOCKFREEHASHMAP_FORCE_INLINE __forceinline
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__OPTIMIZE__)
#define LOCKFREEHASHMAP_FORCE_INLINE inline __attribute__((always_inline))
#else
#define LOCKFREEHASHMAP_FORCE_INLINE inline
#endif