#pragma once
#include <map>
#include <stdint.h>
#include "HandlePackage.h"

/** 句柄管理
*/
class HandleManager
{
protected:
	/** 构造函数
	*/
	HandleManager();

	/** 析构函数
	*/
	~HandleManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static HandleManager& instance();

public:
	//打开发送端句柄
	void openSendHandle(int32_t pid);
	//创建读取端句柄
	void createReadHandle(int32_t pid);
	//分配句柄
	HANDLE assignHandle(int32_t pid, bool isSend);
	//创建数据内存句柄
	HANDLE createDataHandle(int32_t pid, bool isSend);
	//数据内存创建完毕句柄
	HANDLE createDataEndHandle(int32_t pid, bool isSend);
	//创建钥匙内存句柄
	HANDLE createKeyHandle(int32_t pid, bool isSend);
	//钥匙内存创建完毕句柄
	HANDLE createKeyEndHandle(int32_t pid, bool isSend);
	//删除数据内存句柄
	HANDLE deleteDataHandle(int32_t pid, bool isSend);
	//删除数据内存完毕句柄
	HANDLE deleteDataEndHandle(int32_t pid, bool isSend);

private:
	std::map<int32_t, SendHandlePackage> m_sendHandleMap;
	ReadHandlePackage m_readHandle;
	int32_t m_pid;
};