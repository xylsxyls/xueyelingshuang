#include "SendTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"

SendTask::SendTask() :
m_dest(nullptr),
m_buffer(nullptr),
m_length(0),
m_libuvTcp(nullptr)
{

}

void SendTask::setLibuvTcp(LibuvTcp* libuvTcp)
{
	m_libuvTcp = libuvTcp;
}

void SendTask::setParam(uv_tcp_t* dest, char* buffer, int32_t length)
{
	m_dest = dest;
	m_buffer = buffer;
	m_length = length;
}

void SendTask::DoTask()
{
	m_libuvTcp->send(m_dest, m_buffer, m_length);
	if (m_length == 0)
	{
		return;
	}
	::free(m_buffer);
}