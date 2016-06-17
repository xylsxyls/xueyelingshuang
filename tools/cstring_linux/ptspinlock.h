#include <assert.h>

#if defined(_WIN32)
    #error windows not support spinlock operate.
#elif defined(__GNUC__)
    #include <pthread.h>
#else
    #error use must add yourself code.
#endif

#ifndef __PTSPINLOCK_H__
#define __PTSPINLOCK_H__

class ptSpinLock  // 建立一个自旋锁
{
public:
    ptSpinLock(int pshared=0)   // 初始化一个锁, pshared!=0时，可进程共享
    {
    #if defined(__GNUC__)
        pthread_spin_init(&SpinLock, pshared);
    #endif
    }

    virtual ~ptSpinLock()      // 删除锁
    {
    #if defined(__GNUC__)
        pthread_spin_destroy(&SpinLock);
    #endif
    }

    inline void Lock()      // 锁定
    {
    #if defined(__GNUC__)
        pthread_spin_lock(&SpinLock);
    #endif
    }

    inline void unLock()    // 解锁
    {
    #if defined(__GNUC__)
        pthread_spin_unlock(&SpinLock);
    #endif
    }

    inline bool tryLock()    // 试图加锁，非阻塞
    {
    #if defined(__GNUC__)
        return (pthread_spin_trylock(&SpinLock)==0);
    #endif
    }

private:
#if defined(__GNUC__)
    pthread_spinlock_t SpinLock;
#endif
};

#endif


#ifndef __PTSPINLOCKADP_H__
#define __PTSPINLOCKADP_H__

class ptSpinLockAdp   // 对自旋锁的一个简单的包装
{
public:
    ptSpinLockAdp(ptSpinLock* SpinLock, bool bInitialLock=true)
    {
        assert(SpinLock != NULL);
        pSpin = SpinLock;
        if(bInitialLock)
        {
            islock = false;
            this->Lock();
        }
    }

    virtual ~ptSpinLockAdp()
    {
        this->unLock();
    }

    inline void Lock()
    {
        if(!islock)
        {
            pSpin->Lock();
            islock = true;
        }
    }

    inline void unLock()
    {
        if(islock)
        {
            pSpin->unLock();
            islock = false;
        }
    }

    inline bool tryLock()
    {
        islock = islock ? true : pSpin->tryLock();
        return islock;
    }

    inline bool isLocked() { return islock; }

private:
    ptSpinLock* pSpin;
    bool islock;
};

#endif

