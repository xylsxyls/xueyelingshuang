#include "PlaybackEngineTypes.h"
#include "LoopPlayerConstants.h"

namespace LoopPlayer
{
    VideoTimelineInfo::VideoTimelineInfo()
        : isValid(false),
          sampleCount(0),
          result(E_FAIL),
          firstTime(-1),
          secondTime(-1),
          lastTime(-1),
          lastEndTime(0),
          firstDuration(0),
          sourceDuration(0),
          maxGap(0),
          maxGapStart(-1),
          maxGapEnd(-1)
    {
    }

    PlaybackEngineInitParam::PlaybackEngineInitParam()
        : m_videoWindow(nullptr),
          m_eventWindow(nullptr),
          m_eventMessage(0),
          m_videoTimelineOffset(0),
          m_fallbackFrameDuration(DEFAULT_FRAME_DURATION)
    {
    }

    PlaybackVideoFrame::PlaybackVideoFrame()
        : m_width(0),
          m_height(0),
          m_stride(0),
          m_time(0),
          m_duration(0)
    {
    }

    PlaybackAudioBuffer::PlaybackAudioBuffer()
        : m_prepared(false)
    {
        ZeroMemory(&m_header, sizeof(m_header));
    }

    PlaybackEngineSnapshot::PlaybackEngineSnapshot()
        : m_state(PlaybackEngineStateClosed),
          m_lastResult(S_OK),
          m_position(0),
          m_duration(0),
          m_frameDuration(DEFAULT_FRAME_DURATION),
          m_playbackRate(1.0),
          m_videoAvailable(false),
          m_audioAvailable(false),
          m_videoEnded(false),
          m_audioEnded(false)
    {
    }

    const wchar_t* PlaybackEngineStateName(PlaybackEngineState state)
    {
        switch (state)
        {
        case PlaybackEngineStateClosed:
            return L"已关闭";
        case PlaybackEngineStateStopped:
            return L"已停止";
        case PlaybackEngineStatePaused:
            return L"已暂停";
        case PlaybackEngineStatePlaying:
            return L"正在播放";
        case PlaybackEngineStateEnded:
            return L"已结束";
        case PlaybackEngineStateError:
            return L"错误";
        default:
            return L"未知";
        }
    }

    const wchar_t* PlaybackEngineEventName(PlaybackEngineEvent eventType)
    {
        switch (eventType)
        {
        case PlaybackEngineEventNone:
            return L"无";
        case PlaybackEngineEventPositionSet:
            return L"位置已设置";
        case PlaybackEngineEventEnded:
            return L"播放结束";
        case PlaybackEngineEventError:
            return L"错误";
        default:
            return L"未知";
        }
    }
}
