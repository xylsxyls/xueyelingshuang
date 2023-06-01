#pragma once
#include "CTask.h"
#include "CTaskThread.h"
#include <map>
#include <memory>
#include <atomic>

/* 线程管理类
*/
class CTaskThreadManagerAPI CTaskThreadManager
{
private:
	/* 构造函数
	*/
	CTaskThreadManager();
	
	/* 析构函数
	*/
	~CTaskThreadManager();
	
public:
	/* 单一实例
	@return 返回单一实例
	*/
	static CTaskThreadManager& Instance();

	//返回线程ID
	//windows下不要在动态库的全局变量包含全局变量的构造函数中执行，会在std::thread创建处卡死，在linux下，换成CreateThread，exe中创建，编译成静态库，都不会有问题
	uint32_t Init();

	/* 阻塞等待线程结束
	@param [in] threadId 线程ID
	*/
	void WaitForEnd(uint32_t threadId);

	/* 反初始化
	@param [in] threadId 线程ID
	*/
	void Uninit(uint32_t threadId);

	/* 反初始化所有线程
	*/
	void UninitAll();

	/* 在这里取出线程操作指针，不要长久保存该指针，否则即便线程停止也不会释放资源
	@param [in] threadId 线程ID
	@return 返回线程操作指针
	*/
	std::shared_ptr<CTaskThread> GetThreadInterface(uint32_t threadId);

	/* 线程数量
	@return 返回线程数量
	*/
	int32_t Count();

private:
	uint32_t GetThreadId();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<uint32_t, std::shared_ptr<CTaskThread>> m_spThreadMap;
	/* 线程池锁
	*/
	std::mutex m_mutex;
	std::atomic<uint32_t> m_threadId;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
