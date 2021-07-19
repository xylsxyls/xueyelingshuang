#pragma once
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <memory>
#include <map>
#include <unordered_map>
#include <vector>
#include <atomic>
#include "ProcessWorkMacro.h"

class SharedMemory;
class ProcessReceiveCallback;

/** 进程通信类
*/
class ProcessWorkAPI ProcessWork
{
protected:
	/** 构造函数
	*/
	ProcessWork();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static ProcessWork& instance();

public:
	/** 增加接收回调类，按先入先执行回调顺序
	@param [in] callback 接收回调类
	*/
	void addProcessReceiveCallback(ProcessReceiveCallback* callback);

	/** 初始化接收模块
	@param [in] receiveSize 单个共享内存接收区大小，500K
	@param [in] areaCount 缓存块个数
	@return 返回是否初始化成功
	*/
	bool initReceive(int32_t receiveSize = 500 * 1024, int32_t areaCount = 100);

	/** 销毁接收模块资源
	*/
	void uninitReceive();

	/** 清空发送模块指定pid的资源，不涉及存储pid的缓冲区map
	@param [in] destPid 指定pid
	*/
	void clearDestPid(int32_t destPid);

	/** 清空发送模块不用的缓存资源
	*/
	void clearUseless();

	/** 清空发送模块的所有缓存资源
	*/
	void clear();

	/** 初始化发送线程，不可并行
	*/
	void initPostThread();

	/** 销毁发送线程
	*/
	void uninitPostThread();
	
	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况，该函数执行完则表示数据已写入共享内存并通知对方，需业务保证对方进程存在
	windows下发送速度71.0万/s，接收速度71.0万/s，linux下发送速度45.9万包/s，接收速度45.9万包/s
	@param [in] destPid 目标进程pid
	@param [in] buffer 字符串地址，空包填nullptr
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void send(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送数据
	@param [in] destPid 目标进程pid
	@param [in] message 发送内容，可以为空字符串，等同于nullptr
	@param [in] type 发送数据类型
	*/
	void send(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况，该函数执行完则表示数据已写入共享内存并通知对方
	windows下发送速度67.7万包/s，接收速度67.7万包/s，xp方案为51万包/s，多进程，5进程43.9万包/s，10进程，36.7万包/s，20进程57.2万包/s，linux下发送速度38.4万包/s，接收速度38.4万包/s
	@param [in] processName 目标进程名，windows下不带后缀名，linux下全名
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void send(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送数据
	@param [in] processName 目标进程名，windows下不带后缀名，linux下全名
	@param [in] message 发送内容
	@param [in] type 发送数据类型
	*/
	void send(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，单进程单线程有序，多进程或多线程可能出现先发后到
	@param [in] destPid 目标进程pid
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void post(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，单进程单线程有序，多进程或多线程可能出现先发后到
	@param [in] destPid 目标进程pid
	@param [in] message 发送内容
	@param [in] type 发送数据类型
	*/
	void post(int32_t destPid, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，单进程单线程有序，多进程或多线程可能出现先发后到
	当有11条线程同时调用时，是send的3倍速度，12条线程是send的3分之1速度，超过11条性能大幅下降，1条线程发送速度410.2万包/s
	@param [in] processName 目标进程名，带后缀
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void post(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，单进程单线程有序，多进程或多线程可能出现先发后到
	@param [in] processName 目标进程名，带后缀
	@param [in] message 发送内容
	@param [in] type 发送数据类型
	*/
	void post(const std::string& processName, const std::string& message, MessageType type = MessageType::MESSAGE);

	/** 阻塞函数，等待发送任务全部执行后返回，该函数返回则表示已将数据全部写入共享内存并已通知对方进程，结束后会更换线程号
	*/
	void waitForPostEnd();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::shared_ptr<SharedMemory>> m_memoryMap;
	std::vector<ProcessReceiveCallback*> m_callback;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_thisProcessPid;
	Semaphore* m_readSemaphore;
	SharedMemory* m_areaAssign;
	SharedMemory* m_areaRead;
	SharedMemory* m_pid;
	uint32_t m_scanThreadId;
	uint32_t m_readThreadId;
	uint32_t m_copyThreadId;
	uint32_t m_receiveThreadId;
	uint32_t m_postThreadId;
	uint32_t m_postNameThreadId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	ReadWriteMutex m_sendProcessAssignMapMutex;
	std::map<int32_t, std::shared_ptr<SharedMemory>> m_sendProcessAssignMap;
	ReadWriteMutex m_sendProcessReadMapMutex;
	std::map<int32_t, std::shared_ptr<SharedMemory>> m_sendProcessReadMap;
	ReadWriteMutex m_sendMemoryMapMutex;
	std::unordered_map<std::string, std::shared_ptr<SharedMemory>> m_sendMemoryMap;
	ReadWriteMutex m_sendSemaphoreMapMutex;
	std::map<int32_t, std::shared_ptr<Semaphore>> m_sendSemaphoreMap;
	ReadWriteMutex m_sendNameMemoryMapMutex;
	std::map<std::string, std::pair<int32_t, std::shared_ptr<SharedMemory>>> m_sendNameMemoryMap;

	LockFreeQueue<int32_t> m_assignQueue;
	Semaphore m_assignSemaphore;
	LockFreeQueue<char*> m_receiveQueue;
	Semaphore m_receiveSemaphore;

	ReadWriteMutex m_postMutex;
	std::atomic<bool> m_waitEndPost;
	LockFreeQueue<char*> m_postQueue;
	Semaphore m_postSemaphore;
	ReadWriteMutex m_postNameMutex;
	std::atomic<bool> m_waitEndPostName;
	LockFreeQueue<char*> m_postNameQueue;
	Semaphore m_postNameSemaphore;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};