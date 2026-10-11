#pragma once
#include "ReadWriteMutexBase.h"
#ifdef _MSC_VER
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <pthread.h>
#endif
#include <atomic>

/** 进程内递归互斥锁适配，read和write均独占并共享计数，不提供并发读者语义
*/
class ReadWriteMutexAPI RecursiveReadWriteMutex : public ReadWriteMutexBase
{
public:
	/** 初始化递归互斥锁和零计数
	*/
	RecursiveReadWriteMutex();

	/** 销毁递归锁资源，调用前须停止全部使用者
	*/
	~RecursiveReadWriteMutex();

public:
	/** 按write语义取得一次递归互斥锁，不是共享读锁
	*/
	virtual void read();

	/** 同步取得递归互斥锁，同一线程可重入并增加计数
	*/
	virtual void write();

	/** 按unwrite语义释放一次当前线程持有的递归锁
	*/
	virtual void unread();

	/** 由持有线程释放一次递归锁，计数为零时直接返回
	*/
	virtual void unwrite();

	/** 读取当前递归加锁总次数，仅作瞬时观察
	@return 当前递归加锁次数，不可据此判断线程所有权
	*/
	int lockCount();

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	RecursiveReadWriteMutex(const RecursiveReadWriteMutex& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	RecursiveReadWriteMutex& operator=(const RecursiveReadWriteMutex& other);

private:
#ifdef _MSC_VER
	// 递归互斥锁，仅持有线程可以配对释放
	CRITICAL_SECTION m_lock;
#elif defined(__unix__) || defined(__APPLE__)
	// 递归互斥锁，仅持有线程可以配对释放
	pthread_mutex_t m_lock;
	// POSIX递归互斥锁属性，由本对象初始化和销毁
	pthread_mutexattr_t m_attr;
#endif
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 原子总计数，不代表调用线程拥有锁
	std::atomic<int> m_lockCount;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};