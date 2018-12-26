#pragma once
#include <map>
#include <stdint.h>
#include "LockFreeQueue/LockFreeQueueAPI.h"

struct HandlePackage
{
	//分配句柄
	HANDLE m_assignHandle;
	//创建数据内存句柄
	HANDLE m_createDataHandle;
	//创建数据内存结束句柄
	HANDLE m_createDataEndHandle;
	//创建钥匙内存句柄
	HANDLE m_createKeyHandle;
	//创建钥匙内存结束句柄
	HANDLE m_createKeyEndHandle;
	//删除数据内存句柄
	HANDLE m_deleteDataHandle;
	//删除数据内存结束句柄
	HANDLE m_deleteDataEndHandle;

	/** 构造函数
	*/
	HandlePackage()
	{
		m_assignHandle = nullptr;
		m_createDataHandle = nullptr;
		m_createDataEndHandle = nullptr;
		m_createKeyHandle = nullptr;
		m_createKeyEndHandle = nullptr;
		m_deleteDataHandle = nullptr;
		m_deleteDataEndHandle = nullptr;
	}
};

struct SendHandlePackage : public HandlePackage
{

};

struct ReadHandlePackage : public HandlePackage
{
	
};