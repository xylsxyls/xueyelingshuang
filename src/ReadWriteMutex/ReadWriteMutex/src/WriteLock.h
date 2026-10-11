#pragma once
#include "ReadWriteMutexMacro.h"

class ReadWriteMutexBase;

/** 作用域写锁守卫，构造write、析构unwrite；借用锁必须活到守卫销毁
*/
class ReadWriteMutexAPI WriteLock
{
public:
	/** 借用指定锁并同步取得一次写锁
	@param [in,out] mutex 借用的锁，必须存活到守卫析构并满足配对释放规则
	*/
	WriteLock(ReadWriteMutexBase& mutex);

	/** 调用借用锁的unwrite，与构造配对；不销毁锁对象
	*/
	~WriteLock();

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	WriteLock(const WriteLock& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	WriteLock& operator=(const WriteLock& other);

private:
	// 借用的锁对象，必须比本守卫存活更久
	ReadWriteMutexBase* m_mutex;
};