#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "PlayerPreviewTask.h"
#include "PlayerEngine.h"

PlayerPreviewTask::PlayerPreviewTask(PlayerEngine* engine) :
CTask(PlayerPreviewTask::kTaskId),
m_engine(engine),
m_pending(false),
m_position100ns(0),
m_requestSerial(0),
m_exit(false)
{

}

PlayerPreviewTask::~PlayerPreviewTask()
{

}

void PlayerPreviewTask::request(int64_t position100ns, uint64_t requestSerial)
{
	{
		std::lock_guard<std::mutex> lock(m_requestMutex);
		if (m_exit.load() || requestSerial <= m_requestSerial)
		{
			return;
		}
		m_position100ns = position100ns;
		m_requestSerial = requestSerial;
		m_pending = true;
	}
	m_requestReady.notify_one();
}

void PlayerPreviewTask::DoTask()
{
	if (m_exit.load())
	{
		return;
	}
	if (m_engine == nullptr)
	{
		return;
	}
	while (!m_exit.load())
	{
		int64_t position100ns = 0;
		uint64_t requestSerial = 0;
		{
			std::unique_lock<std::mutex> lock(m_requestMutex);
			while (!m_pending && !m_exit.load())
			{
				m_requestReady.wait(lock);
			}
			if (m_exit.load())
			{
				return;
			}
			position100ns = m_position100ns;
			requestSerial = m_requestSerial;
			m_pending = false;
		}
		m_engine->previewFrameFromTask(position100ns, requestSerial, &m_exit);
	}
}

void PlayerPreviewTask::StopTask()
{
	{
		std::lock_guard<std::mutex> lock(m_requestMutex);
		m_exit = true;
		m_pending = false;
	}
	m_requestReady.notify_one();
}