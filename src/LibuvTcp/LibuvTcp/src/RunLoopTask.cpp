#include "libuv/uv.h"
#include "RunLoopTask.h"

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
	int res = uv_run(m_loop, UV_RUN_DEFAULT);
	printf("loop end, res = %d\n", res);
}