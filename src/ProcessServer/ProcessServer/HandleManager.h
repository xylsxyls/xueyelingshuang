#pragma once
#include <map>
#include <stdint.h>

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
	//分配句柄
	HANDLE assignHandle();
	//客户端读取钥匙句柄
	HANDLE clientReadKeyHandle(int32_t pid = 0);
	//客户端读取钥匙完毕句柄
	HANDLE clientReadKeyEndHandle(int32_t pid = 0);
	//创建数据内存句柄
	HANDLE createDataHandle();
	//数据内存创建完毕句柄
	HANDLE createDataEndHandle();
	//创建钥匙内存句柄
	HANDLE createKeyHandle();
	//钥匙内存创建完毕句柄
	HANDLE createKeyEndHandle();
	//删除数据内存句柄
	HANDLE deleteDataHandle();
	//删除数据内存完毕句柄
	HANDLE deleteDataEndHandle();

private:
	HANDLE m_assignHandle;
	HANDLE m_clientReadKeyHandle;
	HANDLE m_clientReadKeyEndHandle;
	HANDLE m_createDataHandle;
	HANDLE m_createDataEndHandle;
	HANDLE m_createKeyHandle;
	HANDLE m_createKeyEndHandle;
	HANDLE m_deleteDataHandle;
	HANDLE m_deleteDataEndHandle;

	std::map<int32_t, HANDLE> m_clientKeyMap;
	std::map<int32_t, HANDLE> m_clientKeyEndMap;
};