#pragma once
#include "ReadWriteMutexMacro.h"
#include "ReadWriteMutexBase.h"

class ReadWriteMutexBase;

/** 作用域读锁守卫，构造read、析构unread；借用锁必须活到守卫销毁
*/
class ReadWriteMutexAPI ReadLock
{
public:
	/** 借用指定锁并同步取得一次读锁
	@param [in,out] mutex 借用的锁，必须存活到守卫析构并满足配对释放规则
	*/
	ReadLock(ReadWriteMutexBase& mutex);

	/** 调用借用锁的unread，与构造配对；不销毁锁对象
	*/
	~ReadLock();

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	ReadLock(const ReadLock& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	ReadLock& operator=(const ReadLock& other);

private:
	// 借用的锁对象，必须比本守卫存活更久
	ReadWriteMutexBase* m_mutex;
};