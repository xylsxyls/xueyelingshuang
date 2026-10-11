#pragma once
#include "ReadWriteMutexMacro.h"

/** 读写锁抽象接口，阻塞、递归和进程范围由具体实现决定；析构前须停止所有使用者
*/
class ReadWriteMutexAPI ReadWriteMutexBase
{
public:
	/** 通过基类指针销毁具体锁，调用前须停止并发访问并释放持有的锁
	*/
	virtual ~ReadWriteMutexBase();

public:
	/** 同步取得读锁，具体实现可以将读操作按独占互斥处理
	*/
	virtual void read() = 0;

	/** 同步取得写锁，不提供从读锁无条件升级的保证
	*/
	virtual void write() = 0;

	/** 释放与read配对的锁，调用线程和持有状态须满足具体实现约束
	*/
	virtual void unread() = 0;

	/** 释放与write配对的锁，调用线程和持有状态须满足具体实现约束
	*/
	virtual void unwrite() = 0;

};