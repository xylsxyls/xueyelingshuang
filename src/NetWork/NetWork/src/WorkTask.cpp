#include "WorkTask.h"
#include "CallbackBase.h"

WorkTask::WorkTask():
m_client(nullptr),
m_buffer(nullptr),
m_length(0),
m_callback(nullptr)
{

}

void WorkTask::setCallback(CallbackBase* callback)
{
	m_callback = callback;
}

void WorkTask::setParam(uv_tcp_t* client, char* buffer, int32_t length)
{
	m_client = client;
	m_buffer = buffer;
	m_length = length;
}

void WorkTask::DoTask()
{
	m_callback->receive(m_client, m_buffer + 4, m_length - 4, *(int32_t*)m_buffer);
	if (m_buffer == nullptr)
	{
		//加了protocolId之后包大小不可能为0
		return;
	}
	::free(m_buffer);
}