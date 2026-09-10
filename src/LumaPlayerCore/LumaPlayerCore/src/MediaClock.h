#pragma once
#include <stdint.h>
#include <chrono>

/** 播放时钟，时钟只保存100纳秒整数时间和整数倍速，避免用double参与播放位置计算
*/
class MediaClock
{
public:
	/** 构造停止状态的时钟
	*/
	MediaClock();

public:
	/** 从指定媒体位置开始走时
	@param [in] position100ns 起始媒体位置，单位100纳秒
	@param [in] ratePermille 播放倍速，由调用方按配置限幅，1000表示1.0倍速，非正值防御性按1处理
	*/
	void start(int64_t position100ns, int32_t ratePermille);

	/** 暂停时钟并把当前走到的位置保存为新的基准位置
	@return 暂停瞬间的媒体位置，单位100纳秒
	*/
	int64_t pause();

	/** 设置当前媒体位置
	@param [in] position100ns 新位置，单位100纳秒
	*/
	void seek(int64_t position100ns);

	/** 设置倍速，运行中的时钟会保持当前播放位置连续
	@param [in] ratePermille 播放倍速，由调用方按配置限幅，1000表示1.0倍速，非正值防御性按1处理
	*/
	void setRate(int32_t ratePermille);

	/** 查询当前媒体位置
	@return 返回当前位置，单位100纳秒
	*/
	int64_t position100ns() const;

	/** 判断时钟是否正在走时
	@return true表示处于播放走时状态
	*/
	bool isRunning() const;

private:
	// 记录开始走时时的媒体位置
	int64_t m_basePosition100ns;
	// 记录开始走时时的单调时钟时间点
	std::chrono::steady_clock::time_point m_startTick;
	// 当前倍速，1000表示1.0倍速
	int32_t m_ratePermille;
	// 当前时钟是否正在走时
	bool m_isRunning;
};