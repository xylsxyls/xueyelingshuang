#include "LumaPlayerMicrophoneChannel.h"

LumaPlayerMicrophoneChannel::LumaPlayerMicrophoneChannel(LumaPlayerCoreBridge* core, int32_t slot) :
m_muted(true),
m_initialized(false),
m_gain(100),
m_device(-1),
m_slot(slot),
m_retired(false),
m_task(&m_capture, core, slot)
{

}

LumaPlayerMicrophoneChannel::~LumaPlayerMicrophoneChannel()
{
    m_task.uninit();
    m_capture.stop();
}