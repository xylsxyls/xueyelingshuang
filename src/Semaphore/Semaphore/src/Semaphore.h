#pragma once
#include <condition_variable>
#include <mutex>
#include "SemaphoreMacro.h"
#ifdef _MSC_VER
#include <windows.h>
#elif __unix__
#include <semaphore.h>
#include <string>
#endif

/** 信号类，包含信号量和事件
*/
class SemaphoreAPI Semaphore
{
public:
	/** 构造函数
	*/
	Semaphore();

	/** 析构函数
	*/
	~Semaphore();

public:
	/** 信号量的通知，通知几次wait允许通过几次，wait之前通知一次则wait执行时允许通过，一对一，不允许一对多
	*/
	void signal();

	/** 信号量的等待
	*/
	void wait();

	/** 信号量的等待
	@param [in] timeout 超时时间，单位毫秒
	@return 如果是参数是-1则一定返回true，如果参数是时间，超时返回false，等待到信号返回true
	*/
	bool wait(int32_t timeout);

	/** 进程信号量的创建
	@param [in] name 名字
	@param [in] signalCount 同时接受的最大信号量个数
	*/
	void createProcessSemaphore(const std::string& name, int32_t signalCount = 800000000);

	/** 打开信号量
	@param [in] name 名字
	*/
	void openProcessSemaphore(const std::string& name);

	/** 关闭信号量，在析构中也会执行，重复执行不会崩溃
	*/
	void closeProcessSemaphore();

	/** 进程信号
	*/
	void processSignal();

	/** 进程等待
	*/
	void processWait();

	/** 进程等待
	@param [in] timeout 超时时间，单位毫秒
	@return 等到信号返回true，超时或未初始化返回false
	*/
	bool processWait(int32_t timeout);

	/** 事件通知，多个事件同时等待只会通知一个，必须进入了eventWait，通知才有效果，一对一，允许一对多
	*/
	void event();

	/** 事件通知，同时通知多个事件，必须进入了eventWait，通知才有效果，一对多
	*/
	void eventAll();

	/** 事件的等待
	*/
	void eventWait();

	/** 事件的等待
	@param [in] timeout 超时时间，单位毫秒
	*/
	bool eventWait(int32_t timeout);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex m_mtx;
	std::condition_variable m_cv;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_count;

#ifdef _MSC_VER
	HANDLE m_processSemaphore;
#elif __unix__
	sem_t* m_processSemaphore;
	std::string m_name;
#endif
};