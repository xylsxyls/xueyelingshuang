#pragma once
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <map>
#include "KeyPackage.h"

class SharedMemory;
/** 共享内存管理类
*/
class SharedMemoryManager
{
protected:
	/** 构造函数
	*/
	SharedMemoryManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static SharedMemoryManager& instance();

public:
	/** 释放资源
	*/
	void uninit();

public:
	/** 增加数据写入位置
	@param [in] pid 进程ID
	@param [in] length 写入的数据长度
	*/
	bool addDataPosition(int32_t pid, int32_t length);

	/** 获取发送数据内存
	@param [in] pid 进程ID
	@return 返回发送数据内存
	*/
	SharedMemory* getCurrentSendData(int32_t pid);

	/** 增加数据内存已经使用的值，最后4字节
	@param [in] pid 进程ID
	@param [in] length 发送数据长度
	*/
	void addDataAlreadyUsed(int32_t pid, int32_t length);

	/** 获取发送数据内存索引值
	@param [in] pid 进程ID
	@return 返回发送数据内存索引值
	*/
	int32_t& sendDataIndex(int32_t pid);

	/** 获取发送数据内存最新发送位置
	@param [in] pid 进程ID
	@return 返回发送数据内存最新发送位置
	*/
	int32_t& sendDataPosition(int32_t pid);

	/** 增加钥匙写入位置
	@param [in] pid 进程ID
	@return 返回是否增加成功
	*/
	bool addKeyPosition(int32_t pid);

	/** 先切换到当前钥匙内存再写入钥匙
	@param [in] pid 进程ID
	@param [in] keyPackage 钥匙内存
	*/
	void writeKey(int32_t pid, const KeyPackage& keyPackage);

	/** 创建数据内存
	*/
	void createData();

	/** 创建钥匙内存
	*/
	void createKey();

	/** 读取钥匙
	@return 返回钥匙
	*/
	KeyPackage readKey();

	/** 读取数据
	@param [out] buffer 数据缓冲区
	@param [in] keyPackage 钥匙
	*/
	void readData(char*& buffer, const KeyPackage& keyPackage);

	/** 减少数据内存使用值并判断是否仍然需要
	@param [in] length 减少的长度
	@return 返回改内存是否仍然需要
	*/
	bool reduceDataValid(int32_t length);

	/** 增加读取钥匙位置
	@return 返回增加后是否仍然在当前内存
	*/
	bool addReadKeyPosition();

	/** 删除数据内存
	@param [in] index 数据内存索引值
	*/
	void deleteData(int32_t index);

	/** 删除钥匙内存
	*/
	void deleteKey();

protected:
	//钥匙内存
	LockFreeQueue<HANDLE> m_keyList;
	//数据内存，内存序号，句柄
	std::map<int32_t, HANDLE> m_dataMap;
};