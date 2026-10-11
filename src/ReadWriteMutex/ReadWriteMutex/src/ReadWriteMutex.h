#pragma once
#include "ReadWriteMutexBase.h"
#include <mutex>

//#define __SUPPORT_XP__

#ifdef _MSC_VER
#include <windows.h>
#ifdef __SUPPORT_XP__
#include <atomic>
#include <condition_variable>
#endif
#elif defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#endif

/** 进程内非递归读写锁，读者可并发、写者独占；析构前须停止全部使用线程
*/
class ReadWriteMutexAPI ReadWriteMutex : public ReadWriteMutexBase
{
public:
	/** 初始化本对象独立的进程内读写锁
	*/
	ReadWriteMutex();

#if defined(__unix__) || defined(__APPLE__)
	/** 销毁POSIX读写锁，调用前须确保没有持有者或等待者
	*/
	virtual ~ReadWriteMutex();

#endif

public:
	/** 同步等待共享读锁，不依赖递归或读写锁升级
	*/
	virtual void read();

	/** 释放当前线程持有的一次读锁，必须与read配对
	*/
	virtual void unread();

	/** 同步等待独占写锁，同一线程不得重复取得写锁
	*/
	virtual void write();

	/** 释放当前线程持有的一次写锁，必须与write配对
	*/
	virtual void unwrite();

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	ReadWriteMutex(const ReadWriteMutex& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	ReadWriteMutex& operator=(const ReadWriteMutex& other);

private:
#ifdef _MSC_VER

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#ifdef __SUPPORT_XP__
	// XP兼容分支的读者计数，负一表示写者持有
	std::atomic<int32_t> m_readCount;
	// Windows为SRWLOCK或XP兼容互斥锁；POSIX为pthread读写锁
	std::mutex m_writeMutex;
	// XP兼容分支中唤醒等待读写权限的线程
	std::condition_variable m_condition;
#else
	// Windows为SRWLOCK或XP兼容互斥锁；POSIX为pthread读写锁
	SRWLOCK m_writeMutex;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#elif defined(__unix__) || defined(__APPLE__)
	// Windows为SRWLOCK或XP兼容互斥锁；POSIX为pthread读写锁
	pthread_rwlock_t m_writeMutex;
#endif
};