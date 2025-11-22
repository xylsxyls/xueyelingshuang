#pragma once
#include <functional>
#include <atomic>
#include <thread>
#include <cstdint>
#include "TimerMacro.h"

/** 无锁定时器类（基于指针+原子变量，无mutex/condition_variable，线程安全）
*  支持Lambda、普通函数、函数对象作为回调，支持动态修改间隔，停止时最多延迟一个间隔
*  所有成员均为指针类型，避免DLL接口导出警告
*/
class TimerAPI Timer
{
public:
	/** 构造函数：初始化定时器回调和定时间隔
	@param [in] callback 定时器触发时执行的回调函数（必须非空）
	@param [in] interval_ms 初始定时间隔（毫秒，必须大于0）
	@throw std::invalid_argument 当callback为空时抛出
	@throw std::bad_alloc 当动态分配资源失败时抛出
	*/
	Timer(std::function<void()> callback, uint64_t interval_ms);

	/** 析构函数：停止定时器、等待工作线程退出、释放所有动态分配的资源
	*/
	~Timer();

	/** 启动定时器（重复触发模式）
	@note 1. 重复调用无效（原子操作保证线程安全，已运行时直接返回并输出警告）
	2. 启动前需确保回调和原子变量已正确初始化
	*/
	void start();

	/** 停止定时器
	@note 1. 重复调用无效，原子操作设置运行状态为false
	2. 停止后线程会在当前睡眠周期结束后退出（最多延迟一个定时间隔）
	3. 若资源未初始化，调用此函数无效果
	*/
	void stop();

	/** 动态修改定时间隔（无锁操作，实时生效）
	@param [in] interval_ms 新的定时间隔（毫秒，必须大于0）
	@note 1. 若输入间隔为0，输出警告并忽略此次修改
	2. 若定时间隔指针未初始化，输出警告并忽略此次修改
	*/
	void set_interval(uint64_t interval_ms);

	/** 检查定时器是否正在运行
	@return true：定时器正在运行；false：定时器已停止或资源未初始化
	*/
	bool is_running() const;

private:
	std::function<void()>* callback_;               // 回调函数指针（需保证线程安全，无数据竞争）
	std::atomic<uint64_t>* interval_ms_;            // 定时间隔（原子变量指针，支持无锁修改）
	std::atomic<bool>* is_running_;                 // 运行状态标记（原子变量指针，线程安全读写）
	std::thread* timer_thread_;                     // 定时器工作线程指针（执行定时等待和回调触发）
};