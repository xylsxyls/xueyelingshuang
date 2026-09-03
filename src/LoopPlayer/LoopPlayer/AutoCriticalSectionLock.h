#pragma once
#include "LoopPlayerPlatform.h"

namespace LoopPlayer
{
    /** CRITICAL_SECTION的轻量作用域锁，用于保证多线程播放管线里成对进入和离开临界区
    */
    class AutoCriticalSectionLock
    {
    public:
        /** 构造时进入临界区
        @param [in] section 需要加锁的临界区，允许为空
        */
        explicit AutoCriticalSectionLock(CRITICAL_SECTION* section);

        /** 析构时离开临界区
        */
        ~AutoCriticalSectionLock();

    private:
        /** 禁止拷贝构造，避免两个锁对象重复释放同一个临界区
        @param [in] other 另一个锁对象
        */
        AutoCriticalSectionLock(const AutoCriticalSectionLock& other);

        /** 禁止赋值，避免锁持有关系被覆盖后无法正确释放
        @param [in] other 另一个锁对象
        @return 返回当前对象引用
        */
        AutoCriticalSectionLock& operator=(const AutoCriticalSectionLock& other);

    private:
        // 当前对象持有的临界区
        CRITICAL_SECTION* m_section;
    };
}