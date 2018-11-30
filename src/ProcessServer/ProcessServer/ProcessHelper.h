#pragma once
#include <string>
#include <stdint.h>
#include "KeyPackage.h"

class SharedMemory;

class ProcessHelper
{
public:
	//位置内存名
	static std::string positionMapName();
	//数据内存名
	static std::string dataMapName(void* position);
	//数据内存名
	static std::string dataMapName(int32_t index);
	//钥匙内存名
	static std::string keyMapName(void* position);
	//钥匙内存名
	static std::string keyMapName(int32_t index);
	//当前数据写入的内存索引值
	static int32_t& dataIndex(void* position);
	//当前数据在内存中的写入位置
	static int32_t& dataPosition(void* position);
	//钥匙写入的内存索引值
	static int32_t& keyIndex(void* position);
	//钥匙在内存中的写入位置
	static int32_t& keyPosition(void* position);
	//位置内存长度
	static int32_t positionLength();
	//数据内存长度
	static int32_t dataMemoryLength();
	//钥匙内存长度
	static int32_t keyMemoryLength();
	//位置锁锁名
	static std::string positionMutexName();
	//切换至当前数据共享内存
	static void changeToCurrentData(SharedMemory** data, SharedMemory* position);
	//创建数据内存
	static SharedMemory* createDataMemory(int32_t index);
	//创建钥匙内存
	static SharedMemory* createKeyMemory(int32_t index);
	//获取删除数据内存索引值
	static int32_t& deleteDataIndex(void* position);
	//向后推移数据写入位置
	static bool addDataPosition(SharedMemory* data, int32_t length);
	//修改当前共享内存已使用大小
	static void addDataAlreadyUsed(SharedMemory* data, int32_t length);
	//切换至当前钥匙内存
	static void changeToCurrentKey(SharedMemory** key, SharedMemory* position);
	//向后推移钥匙写入位置
	static bool addKeyPosition(SharedMemory* key);
	//写入钥匙
	static void writeKey(SharedMemory* key, const KeyPackage& keyPackage);
	//等待信号量最大值
	static int32_t semMaxCount();
	//读取钥匙
	static KeyPackage readKey(SharedMemory* position, SharedMemory* key);
	//读取数据
	static void readData(char*& buffer, const KeyPackage& keyPackage, SharedMemory*& data);
	//减少当前内存使用大小
	static void reduceDataAlreadyUsed(SharedMemory* data, int32_t length);
	//判断是否需要删除共享内存
	static bool needDeleteDataMemory(SharedMemory* data, SharedMemory* position);
	//写入要删除的内存索引值
	static void writeDeleteDataIndex(SharedMemory* position, int32_t index);
	//读取客户端进程ID
	static int32_t readPID(SharedMemory* keyMemory, int32_t position);
	//客户端读取信号量名字
	static std::string clientReadKeyName(int32_t pid);
	//客户端读取完毕信号量名字
	static std::string clientReadKeyEndName(int32_t pid);
	//读取进度索引值
	static int32_t& readKeyIndex(void* position);
	//读取进度位置
	static int32_t& readKeyPosition(void* position);
	//切换到当前读取钥匙内存
	static void changeToCurrentReadKey(SharedMemory** readKey, SharedMemory* position);
	//改变读取钥匙位置
	static void addReadKey(SharedMemory* position);
	//切换到当前读取数据内存
	static void changeToCurrentReadData(SharedMemory** readData, int32_t readIndex);
	//读取钥匙内存名
	static std::string readKeyMapName(void* position);
	//读取数据内存名
	static std::string readDataMapName(void* position);
};