#include "WorkTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetClient.h"
#include "NetServer.h"

WorkTask::WorkTask():
m_sender(nullptr),
m_buffer(nullptr),
m_length(0)
{

}

void WorkTask::DoTask()
{
	MessageType type = (MessageType)(*(int32_t*)m_buffer);
	
	if (m_libuvTcp->isClient())
	{
		if (type == MessageType::HEART)
		{
			((NetClient*)m_libuvTcp)->onHeart();
		}
		else
		{
			((NetClient*)m_libuvTcp)->onReceive(m_buffer + 4, m_length - 4, type);
		}
	}
	else
	{
		if (type == MessageType::HEART)
		{
			((NetServer*)m_libuvTcp)->onHeart();
		}
		else
		{
			((NetServer*)m_libuvTcp)->onReceive(m_sender, m_buffer + 4, m_length - 4, type);
		}
	}

	//if (m_libuvTcp->isClient())
	//{
	//	((NetClient*)m_libuvTcp)->onReceive(m_buffer + 4, m_length - 4, (int32_t)(m_i));
	//}
	//else
	//{
	//	((NetServer*)m_libuvTcp)->onReceive(m_sender, m_buffer + 4, m_length - 4, (int32_t)(m_i));
	//}

	if (m_buffer == nullptr)
	{
		//加了type之后包大小不可能为0
		return;
	}
	::free(m_buffer);
}

void WorkTask::setParam(uv_tcp_t* sender, char* buffer, int32_t length, LibuvTcp* libuvTcp)
{
	m_sender = sender;
	m_buffer = buffer;
	m_length = length;
	m_libuvTcp = libuvTcp;
}