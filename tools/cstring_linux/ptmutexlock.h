#include <assert.h>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__GNUC__)
    #include <pthread.h>
#else
    #error use must add yourself code.
#endif

#ifndef __PTMUTEXLOCK_H__
#define __PTMUTEXLOCK_H__

class ptMutexLock  // 建立一个线程互斥锁
{
public:

#if defined(__GNUC__)
    ptMutexLock(pthread_mutexattr_t* muterattr=NULL)   // 初始化一个锁
#else
    ptMutexLock()   // 初始化一个锁
#endif
    {
    #if defined(_WIN32)
        InitializeCriticalSection(&mutex);
    #elif defined(__GNUC__)
        pthread_mutex_init(&mutex, muterattr);
    #endif
    }

    virtual ~ptMutexLock()      // 删除锁
    {
    #if defined(_WIN32)
        DeleteCriticalSection(&mutex);
    #elif defined(__GNUC__)
        pthread_mutex_destroy(&mutex);
    #endif
    }

    inline void Lock()      // 锁定
    {
    #if defined(_WIN32)
        EnterCriticalSection(&mutex);
    #elif defined(__GNUC__)
        pthread_mutex_lock(&mutex);
    #endif
    }

    inline void unLock()    // 解锁
    {
    #if defined(_WIN32)
        LeaveCriticalSection(&mutex);
    #elif defined(__GNUC__)
        pthread_mutex_unlock(&mutex);
    #endif
    }

#if !defined(_WIN32) || (_WIN32_WINNT >= 0x0400)
    inline bool tryLock()    // 试图加锁，非阻塞
    {
    #if defined(_WIN32) 
        return (!!TryEnterCriticalSection(&mutex)); // 95/98不支持
    #elif defined(__GNUC__)
        return (!!(pthread_mutex_trylock(&mutex)==0));
    #endif
    }
#endif

private:
#if defined(_WIN32)
    CRITICAL_SECTION mutex;
#elif defined(__GNUC__)
    pthread_mutex_t mutex;
#endif
};

#endif


#ifndef __PTMUTEXLOCKADP_H__
#define __PTMUTEXLOCKADP_H__

class ptMutexLockAdp   // 对互斥锁的一个简单的包装,不支持多级锁定
{
public:
    ptMutexLockAdp(ptMutexLock* mutex, bool bInitialLock=true)
    {
        assert(mutex != NULL);
        pMutex = mutex;
        if(bInitialLock) 
        {
            islock = false;
            this->Lock();
        }   
    }

    virtual ~ptMutexLockAdp()
    {
        this->unLock();
    }

    inline void Lock()
    {
        if(!islock)
        {
            pMutex->Lock();
            islock = true;
        }
    }

    inline void unLock()
    {
        if(islock)
        {
            pMutex->unLock();
            islock = false;
        }
    }

#if !defined(_WIN32) || (_WIN32_WINNT >= 0x0400)
    inline bool tryLock()
    {
        islock = islock ? true : pMutex->tryLock();
        return islock;
    }
#endif

    inline bool isLocked() { return islock; }

private:
    ptMutexLock* pMutex;
    bool islock;
};

#endif

