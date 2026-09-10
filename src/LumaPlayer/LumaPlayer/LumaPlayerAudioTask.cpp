#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerAudioTask.h"
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerLogger.h"

#include <QEventLoop>

LumaPlayerAudioTask::LumaPlayerAudioTask(LumaPlayerAudioRender* render, int32_t intervalMs) :
CTask(1),
m_render(render),
m_intervalMs(intervalMs),
m_exit(false)
{

}

void LumaPlayerAudioTask::DoTask()
{
	if (m_exit.load())
	{
		return;
	}
	if (m_render == nullptr)
	{
		return;
	}
	try
	{
		// 为任务管理器创建的线程建立Qt事件分发器，音频后端定时器也在此线程处理
		QEventLoop events;
		while (!m_exit.load())
		{
			events.processEvents(QEventLoop::AllEvents);
			m_render->processAudioQueue();
			m_wake.wait(m_intervalMs);
		}
	}
	catch (...)
	{
		LumaPlayerLogger::log("音频设备服务异常，停止设备输出");
	}
	m_render->shutdownInAudioThread();
}

void LumaPlayerAudioTask::StopTask()
{
	m_exit = true;
	m_wake.signal();
}