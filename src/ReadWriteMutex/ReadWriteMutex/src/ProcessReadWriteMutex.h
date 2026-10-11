#pragma once
#include "ReadWriteMutexMacro.h"
#include <string>
#include "ReadWriteMutexBase.h"

#if defined(__unix__) || defined(__APPLE__)
#include "FileReadWriteMutex.h"
#endif

#ifdef _MSC_VER
typedef void* HANDLE;

/** 命名跨进程互斥锁，Windows使用Mutex，POSIX复用文件锁；读写均为独占语义
*/
class ReadWriteMutexAPI ProcessReadWriteMutex : public ReadWriteMutexBase
{
public:
	/** 创建或打开命名互斥资源，构造时不加锁
	@param [in] name 同一命名空间内同名实例共享资源；POSIX按文件锁路径规则解释
	*/
	ProcessReadWriteMutex(const std::string& name);

	/** 关闭Windows互斥句柄，调用前须配对释放并停止并发访问
	*/
	virtual ~ProcessReadWriteMutex();

public:
	/** 同步等待Windows命名互斥锁，读操作同样独占
	*/
	virtual void read();

	/** 由持有线程释放一次命名互斥锁，与read配对
	*/
	virtual void unread();

	/** 同步等待Windows命名互斥锁，沿用系统Mutex同线程递归语义
	*/
	virtual void write();

	/** 由持有线程释放一次命名互斥锁，与write配对
	*/
	virtual void unwrite();

	/** 以零等待尝试获取Windows命名互斥锁，成功后须由当前线程释放
	@return true仅表示WAIT_OBJECT_0，其他等待状态为false，不区分具体原因
	*/
	bool trywrite();

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	ProcessReadWriteMutex(const ProcessReadWriteMutex& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	ProcessReadWriteMutex& operator=(const ProcessReadWriteMutex& other);

private:
	// 本对象拥有的Windows互斥句柄，创建失败为nullptr
	HANDLE m_hLock;
};
#elif defined(__unix__) || defined(__APPLE__)

/** 命名跨进程互斥锁，Windows使用Mutex，POSIX复用文件锁；读写均为独占语义
*/
class ReadWriteMutexAPI ProcessReadWriteMutex : public FileReadWriteMutex
{
public:
	/** 创建或打开命名互斥资源，构造时不加锁
	@param [in] name 同一命名空间内同名实例共享资源；POSIX按文件锁路径规则解释
	*/
	ProcessReadWriteMutex(const std::string& name);

private:
	/** 禁用拷贝，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	*/
	ProcessReadWriteMutex(const ProcessReadWriteMutex& other);

	/** 禁用赋值，避免复制锁资源或作用域释放责任
	@param [in] other 不允许复制的源对象
	@return 本对象引用；该函数仅声明以禁止赋值
	*/
	ProcessReadWriteMutex& operator=(const ProcessReadWriteMutex& other);

};

#endif