#include "libuv/uv.h"
#include "RunLoopTask.h"
#include "CSystem/CSystemAPI.h"
#include "D:\\SendToMessageTest.h"

RunLoopTask::RunLoopTask() :
m_loop(nullptr)
{

}

void RunLoopTask::setLoop(uv_loop_t* loop)
{
	m_loop = loop;
}

void RunLoopTask::DoTask()
{
	RCSend("loop threadId = %d", CSystem::SystemThreadId());
	int res = uv_run(m_loop, UV_RUN_DEFAULT);
	printf("loop end, res = %d\n", res);
	RCSend("loop end threadId = %d, res = %d", CSystem::SystemThreadId(), res);
}