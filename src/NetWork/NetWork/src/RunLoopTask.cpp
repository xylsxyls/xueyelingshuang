#include "RunLoopTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"

RunLoopTask::RunLoopTask() :
m_libuvTcp(nullptr)
{

}

void RunLoopTask::setParam(LibuvTcp* libuvTcp)
{
	m_libuvTcp = libuvTcp;
}

void RunLoopTask::DoTask()
{
	m_libuvTcp->loop();
}

void RunLoopTask::StopTask()
{
	m_libuvTcp->stop();
}