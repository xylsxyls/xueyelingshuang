#include "SendTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CSystem/CSystemAPI.h"
#include "NetWorkThreadManager.h"

std::mutex g_sendMutex;

SendTask::SendTask() :
m_text(nullptr),
m_libuvTcp(nullptr)
{

}

void SendTask::setLibuvTcp(LibuvTcp* libuvTcp)
{
	m_libuvTcp = libuvTcp;
}

void SendTask::setParam(char* text)
{
	m_text = text;
}

void SendTask::DoTask()
{
	m_libuvTcp->send(m_text);
}