#pragma once
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <map>
#include "KeyPackage.h"
#include "MemroyPackage.h"
#include <mutex>

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
	@return 返回内存是否存在
	*/
	bool addDataAlreadyUsed(int32_t pid, int32_t length);

	/** 获取发送数据内存索引值
	@param [in] pid 进程ID
	@return 返回发送数据内存索引值
	*/
	int32_t sendDataIndex(int32_t pid);

	/** 获取发送数据内存最新发送位置
	@param [in] pid 进程ID
	@return 返回发送数据内存最新发送位置
	*/
	int32_t sendDataPosition(int32_t pid);

	/** 增加钥匙写入位置
	@param [in] pid 进程ID
	@return 返回是否增加成功
	*/
	bool addKeyPosition(int32_t pid);

	/** 提示钥匙一级，进入下一个内存段
	@param [in] pid 进程ID
	@return 返回是否提升成功
	*/
	bool raiseSendKey(int32_t pid);

	/** 提示数据一级，进入下一个内存段
	@param [in] pid 进程ID
	@param [in] length 数据长度
	@return 返回是否提升成功
	*/
	bool raiseSendData(int32_t pid, int32_t length);

	/** 先切换到当前钥匙内存再写入钥匙
	@param [in] pid 进程ID
	@param [in] keyPackage 钥匙内存
	@return 返回内存是否存在
	*/
	bool writeKey(int32_t pid, const KeyPackage& keyPackage);

	/** 初始化接收相关内存
	*/
	void initReceiveMemory();

	/** 创建数据内存
	*/
	void createData();

	/** 创建钥匙内存
	*/
	void createKey();

	/** 读取钥匙，如果进入了下一段内存则传出上一段钥匙内存
	@param [out] keyPackage 钥匙
	@param [out] deleteMemory 删除的内存
	@return 返回是否读取成功
	*/
	bool readKey(KeyPackage& keyPackage, SharedMemory*& deleteMemory);

	/** 读取数据
	@param [out] buffer 数据缓冲区
	@param [in] keyPackage 钥匙
	*/
	void readData(char*& buffer, const KeyPackage& keyPackage);

	/** 减少数据内存使用值并判断是否仍然需要
	@param [in] index 数据内存索引值
	@param [in] length 减少的长度
	@return 返回改内存是否仍然需要
	*/
	bool reduceDataValid(int32_t index, int32_t length);

	/** 增加读取钥匙位置
	@param [out] deleteMemory 要删除的内存
	@return 返回增加后是否仍然在当前内存
	*/
	//bool addReadKeyPosition(SharedMemory*& deleteMemory);

	/** 删除数据内存
	@param [in] index 数据内存索引值
	*/
	void deleteData(int32_t index);

	/** 删除钥匙内存
	@param [in] deleteMemory 要删除的内存指针
	*/
	void deleteKey(SharedMemory* deleteMemory);

protected:
	/** 创建发送内存
	@param [in] pid 发送进程ID
	@return 返回发送进程是否存在
	*/
	bool createSendMemory(int32_t pid);

	/** 获取发送位置内存
	@param [in] pid 发送进程ID
	@return 返回发送位置内存
	*/
	void* getPositionMemory(int32_t pid);

	/** 获取发送数据内存
	@param [in] pid 发送进程ID
	@return 返回发送数据内存
	*/
	void* getDataMemory(int32_t pid);

	/** 获取发送钥匙内存
	@param [in] pid 发送进程ID
	@return 返回发送钥匙内存
	*/
	void* getKeyMemory(int32_t pid);

protected:
	//钥匙内存
	LockFreeQueue<SharedMemory*> m_keyList;
	//数据内存
	std::map<int32_t, SharedMemory*> m_dataMap;
	//位置内存
	SharedMemory* m_position;
	//当前读取的钥匙内存
	SharedMemory* m_readKey;
	//发送内存map
	std::map<int32_t, MemoryPackage> m_send;
	//本进程ID
	int32_t m_pid;
	//读取钥匙内存索引值
	int32_t m_readIndex;
	//读取钥匙位置
	int32_t m_readPosition;
	//发送map锁
	std::mutex m_sendMutex;
	//读取map锁
	std::mutex m_readMutex;
};