#include "TestCommandTask.h"

TestCommandTask::TestCommandTask(const std::function<void(const std::atomic<bool>&)>& command) :
m_exit(false),
m_command(command)
{

}

void TestCommandTask::DoTask()
{
    // 即使已取消也通知命令，由其返回取消结果，避免GUI永久等待。
    m_command(m_exit);
}

void TestCommandTask::StopTask()
{
    m_exit.store(true);
}