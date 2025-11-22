#include "Timer.h"
#include <chrono>
#include <exception>
#include <iostream>
#include <utility>

/** 构造函数实现：初始化回调指针、动态分配原子变量和线程指针
*  1. 校验回调有效性，为空则抛出异常
*  2. 动态分配std::function对象、原子变量和线程指针
*  3. 初始化定时间隔，非法间隔设置默认值并警告
*  @param [in] callback 定时器回调函数（必须非空）
*  @param [in] interval_ms 初始定时间隔（毫秒）
*  @throw std::invalid_argument 当callback为空时抛出
*  @throw std::bad_alloc 当new分配资源失败时抛出
*/
Timer::Timer(std::function<void()> callback, uint64_t interval_ms)
	: callback_(nullptr)
	, interval_ms_(nullptr)
	, is_running_(nullptr)
	, timer_thread_(nullptr)
{
	// 校验回调有效性（原始回调为空则抛出异常）
	if (!callback)
	{
		throw std::invalid_argument("Timer: callback cannot be empty!");
	}

	// 动态分配回调函数对象（转移所有权，避免拷贝）
	callback_ = new std::function<void()>(std::move(callback));

	// 动态分配原子变量（分配失败会抛出std::bad_alloc）
	interval_ms_ = new std::atomic<uint64_t>(interval_ms);
	is_running_ = new std::atomic<bool>(false);

	// 初始化线程指针为nullptr（start()中创建线程）
	timer_thread_ = nullptr;

	// 检查初始间隔有效性，无效则设置默认1000ms并警告
	if (interval_ms == 0)
	{
		std::cerr << "Timer: initial interval cannot be 0, set to 1000ms by default!" << std::endl;
		interval_ms_->store(1000, std::memory_order_release);
	}
}

/** 析构函数实现：停止定时器、释放所有动态资源
*  1. 停止定时器运行
*  2. 等待工作线程结束（若线程已创建且可连接）
*  3. 释放回调函数、原子变量和线程的动态内存
*  4. 置空所有指针，避免野指针访问
*/
Timer::~Timer()
{
	stop();

	// 等待线程结束（若线程指针非空且可连接）
	if (timer_thread_ != nullptr && timer_thread_->joinable())
	{
		timer_thread_->join();
	}

	// 释放动态分配的资源（按创建顺序逆序释放）
	delete callback_;
	delete interval_ms_;
	delete is_running_;
	delete timer_thread_;

	// 置空所有指针，避免野指针访问
	callback_ = nullptr;
	interval_ms_ = nullptr;
	is_running_ = nullptr;
	timer_thread_ = nullptr;
}

/** 启动定时器实现：原子操作避免重复启动，动态创建工作线程
*  1. 校验核心资源是否初始化完成
*  2. 原子操作检查并设置运行状态，避免重复启动
*  3. 动态创建工作线程，执行定时循环逻辑（等待间隔->执行回调）
*  @note 1. 若核心资源未初始化，输出警告并返回
*        2. 若线程创建失败会抛出std::system_error
*/
void Timer::start()
{
	// 校验核心资源是否初始化（回调、间隔、运行状态指针均需非空）
	if (callback_ == nullptr || interval_ms_ == nullptr || is_running_ == nullptr)
	{
		std::cerr << "Timer: core resources not initialized, cannot start!" << std::endl;
		return;
	}

	// 校验回调函数是否有效
	if (!(*callback_))
	{
		std::cerr << "Timer: callback is invalid, cannot start!" << std::endl;
		return;
	}

	// 原子操作：设置为true并返回旧值（避免多线程重复启动）
	if (is_running_->exchange(true))
	{
		std::cerr << "Timer: already running!" << std::endl;
		return;
	}

	// 动态创建工作线程（创建失败会抛出std::system_error）
	timer_thread_ = new std::thread([this]()
	{
		// 循环：等待间隔 -> 执行回调（直到停止信号）
		while (is_running_->load(std::memory_order_acquire))
		{
			// 原子读取当前间隔，无锁等待
			std::this_thread::sleep_for(std::chrono::milliseconds(
				interval_ms_->load(std::memory_order_acquire)
				));

			// 等待后再次检查状态，避免sleep期间被停止后误执行回调
			if (!is_running_->load(std::memory_order_acquire))
			{
				break;
			}

			// 再次校验回调指针和函数有效性，避免野指针访问
			if (callback_ != nullptr && *callback_)
			{
				// 执行回调函数（捕获所有异常，避免线程崩溃）
				try
				{
					(*callback_)();
				}
				catch (const std::exception& e)
				{
					std::cerr << "Timer callback exception: " << e.what() << std::endl;
				}
				catch (...)
				{
					std::cerr << "Timer callback unknown exception!" << std::endl;
				}
			}
			else
			{
				std::cerr << "Timer: callback is null or invalid, skip execution!" << std::endl;
				break; // 回调无效，退出线程循环
			}
		}
	});
}

/** 停止定时器实现：原子设置运行状态为false
*  无锁操作，立即生效；线程会在下次循环检查时退出
*  @note 若运行状态指针未初始化，输出警告并返回
*/
void Timer::stop()
{
	if (is_running_ != nullptr)
	{
		is_running_->store(false, std::memory_order_release);
	}
	else
	{
		std::cerr << "Timer: is_running_ not initialized, stop failed!" << std::endl;
	}
}

/** 动态修改间隔实现：原子存储新间隔
*  无锁操作，下次等待时立即生效；输入非法则输出警告
*  @param [in] interval_ms 新的定时间隔（毫秒）
*  @note 1. 若输入间隔为0，输出警告并忽略此次修改
*        2. 若定时间隔指针未初始化，输出警告并忽略此次修改
*/
void Timer::set_interval(uint64_t interval_ms)
{
	if (interval_ms == 0)
	{
		std::cerr << "Timer: interval cannot be 0!" << std::endl;
		return;
	}

	if (interval_ms_ != nullptr)
	{
		interval_ms_->store(interval_ms, std::memory_order_release);
	}
	else
	{
		std::cerr << "Timer: interval_ms_ not initialized, set_interval failed!" << std::endl;
	}
}

/** 检查运行状态实现：原子加载状态
*  无锁操作，线程安全，返回当前运行状态
*  @return true：运行中；false：已停止或资源未初始化
*/
bool Timer::is_running() const
{
	if (is_running_ != nullptr)
	{
		return is_running_->load(std::memory_order_acquire);
	}
	return false;
}

//int32_t main()
//{
//	std::atomic<int> lambda_count(0);
//	Timer lambda_timer([&lambda_count, &strategyCount]()
//	{
//		if (g_config.m_completeTaskCount == 0)
//		{
//			RCSend("complete = 0");
//			return;
//		}
//		++lambda_count;
//		RCSend("complete = %u, remain = %.1lfs", g_config.m_completeTaskCount,
//			(strategyCount - g_config.m_completeTaskCount) / (double)(g_config.m_completeTaskCount / lambda_count));
//	}, 1000);
//	lambda_timer.start();
//	return 0;
//}