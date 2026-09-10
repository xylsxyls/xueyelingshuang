#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

#include <atomic>
#include <stdint.h>

class LumaPlayerAudioRender;

/** 音频设备服务任务，Qt设备的创建、事件处理和销毁始终在同一线程
*/
class LumaPlayerAudioTask : public CTask
{
public:
	/** 构造任务，不打开设备
	@param [in] render 借用的渲染器，必须存活到任务退出
	@param [in] intervalMs 服务间隔，单位毫秒，必须大于0
	*/
	LumaPlayerAudioTask(LumaPlayerAudioRender* render, int32_t intervalMs);

	/** 运行Qt事件处理和音频队列服务，退出前销毁设备
	*/
	virtual void DoTask();

	/** 请求退出并唤醒等待
	*/
	virtual void StopTask();

private:
	// 借用的音频渲染器
	LumaPlayerAudioRender* m_render;
	// 服务间隔，单位毫秒
	int32_t m_intervalMs;
	// 停止标记
	std::atomic<bool> m_exit;
	// 停止时唤醒服务等待
	Semaphore m_wake;
};