#include "LumaPlayerMicrophoneRetireTask.h"

LumaPlayerMicrophoneRetireTask::LumaPlayerMicrophoneRetireTask(
    const std::shared_ptr<LumaPlayerMicrophoneChannel>& channel) :
m_channel(channel)
{

}

void LumaPlayerMicrophoneRetireTask::DoTask()
{
    m_channel->m_task.uninit();
    m_channel->m_capture.stop();
    m_channel->m_retired.store(true);
}

void LumaPlayerMicrophoneRetireTask::StopTask()
{

}