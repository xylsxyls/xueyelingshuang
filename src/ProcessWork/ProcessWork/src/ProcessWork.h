#pragma once
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>
#include <map>
#include <atomic>
#include <mutex>
#include <vector>
#include "ProcessWorkMacro.h"
#include "FiniteDeque/FiniteDequeAPI.h"

class SharedMemory;
class ProcessReceiveCallback;
class Semaphore;
class ProcessReadWriteMutex;

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
	@param [in] flow 缓存资源数，一般向几个进程发就给几个资源
	@return 返回是否初始化成功
	*/
	bool initReceive(int32_t receiveSize = 500 * 1024, int32_t areaCount = 100, int32_t flow = 3);

	/** 销毁接收模块资源
	*/
	void uninitReceive();

	/** 清空发送模块的缓存资源
	*/
	void clear();

	/** 初始化发送线程，不可并行
	*/
	void initPostThread();

	/** 销毁发送线程
	*/
	void uninitPostThread();
	
	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况，该函数执行完则表示数据已写入共享内存并通知对方，需业务保证对方进程存在
	windows下发送速度1.2万/s，接收速度1.2万/s，linux下发送速度1.6万包/s，接收速度1.6万包/s
	@param [in] destPid 目标进程pid
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void send(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况，该函数执行完则表示数据已写入共享内存并通知对方
	windows下发送速度5万包/s，接收速度2.5万包/s，linux下发送速度1.2万包/s，接收速度1.2万包/s
	@param [in] processName 目标进程名，windows下不带后缀名，linux下全名
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void send(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况
	@param [in] destPid 目标进程pid
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void post(int32_t destPid, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 向服务端发送字符串，单进程有序，多进程无序，有先发后到的情况
	@param [in] processName 目标进程名，带后缀
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] type 发送数据类型
	*/
	void post(const std::string& processName, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	/** 阻塞函数，等待发送任务全部执行后返回，该函数返回则表示已将数据全部写入共享内存并已通知对方进程，结束后会更换线程号
	*/
	void waitForPostEnd();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>> m_memoryMap;
	std::mutex m_assignMutex;
	std::mutex m_readMutex;
	std::vector<ProcessReceiveCallback*> m_callback;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_thisProcessPid;
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	Semaphore* m_readSemaphore;
	Semaphore* m_readEndSemaphore;
	SharedMemory* m_area;
	SharedMemory* m_pid;
	uint32_t m_assignThreadId;
	uint32_t m_readThreadId;
	uint32_t m_copyThreadId;
	uint32_t m_receiveThreadId;
	uint32_t m_postThreadId;

	int32_t m_flow;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	FiniteDeque<std::pair<int32_t, std::shared_ptr<SharedMemory>>> m_sendProcessDeque;
	std::mutex m_sendProcessDequeMutex;
	FiniteDeque<std::pair<std::string, std::shared_ptr<SharedMemory>>> m_sendMemoryDeque;
	std::mutex m_sendMemoryDequeMutex;
	FiniteDeque<std::pair<int32_t, std::shared_ptr<ProcessReadWriteMutex>[2]>> m_sendMutexDeque;
	std::mutex m_sendMutexDequeMutex;
	FiniteDeque<std::pair<int32_t, std::shared_ptr<Semaphore>[4]>> m_sendSemaphoreDeque;
	std::mutex m_sendSemaphoreDequeMutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};