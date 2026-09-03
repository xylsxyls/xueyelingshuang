#include "MfSourcePlaybackEngine.h"
#include "AutoCriticalSectionLock.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"

#include <algorithm>

namespace LoopPlayer
{
    // waveOut循环使用的音频缓冲数量，过小容易断音，过大则seek时清空延迟变长
    static const DWORD kAudioBufferCount = 6;
    // 播放线程等待状态切换的短轮询间隔
    static const DWORD kThreadWaitMs = 10;
    // 视频线程等待目标显示时间的切片间隔
    static const DWORD kVideoWaitSliceMs = 2;
    // 视频播放时至少保留的重排帧数量，异常MP4和H.264 B帧常见读取顺序早于显示顺序
    static const size_t kVideoReorderFrameCount = 8;
    // seek预览最多向前解码的帧数，兼顾关键帧回退后的准确预览和拖动响应
    static const size_t kVideoPreviewMaxReadCount = 90;

    /** 比较两个视频帧的显示时间
    @param [in] left 左侧视频帧
    @param [in] right 右侧视频帧
    @return left时间更早返回true，否则返回false
    */
    static bool VideoFrameTimeLess(const PlaybackVideoFrame& left, const PlaybackVideoFrame& right)
    {
        return left.m_time < right.m_time;
    }

    MfSourcePlaybackEngine::MfSourcePlaybackEngine()
        : m_videoWindow(nullptr),
          m_eventWindow(nullptr),
          m_eventMessage(0),
          m_videoReader(nullptr),
          m_audioReader(nullptr),
          m_videoThread(nullptr),
          m_audioThread(nullptr),
          m_wakeupEvent(nullptr),
          m_exitEvent(nullptr),
          m_waveOut(nullptr),
          m_nextAudioBuffer(0),
          m_isInit(false),
          m_isExit(false),
          m_videoAvailable(false),
          m_audioAvailable(false),
          m_videoEnded(false),
          m_audioEnded(false),
          m_videoSourceEnded(false),
          m_state(PlaybackEngineStateClosed),
          m_lastResult(S_OK),
          m_seekSerial(0),
          m_duration(0),
          m_clockBasePosition(0),
          m_frameDuration(DEFAULT_FRAME_DURATION),
          m_videoTimelineOffset(0),
          m_playbackRate(1.0),
          m_videoWidth(0),
          m_videoHeight(0),
          m_videoStride(0),
          m_videoSampleLogCount(0),
          m_audioSampleLogCount(0),
          m_videoReorderLogCount(0),
          m_lastPublishedVideoTime(-1)
    {
        InitializeCriticalSection(&m_stateLock);
        InitializeCriticalSection(&m_readerLock);
        InitializeCriticalSection(&m_frameLock);
        InitializeCriticalSection(&m_waveLock);
        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));
        ZeroMemory(&m_videoBitmapInfo, sizeof(m_videoBitmapInfo));
        QueryPerformanceFrequency(&m_counterFrequency);
        QueryPerformanceCounter(&m_clockBaseCounter);
    }

    MfSourcePlaybackEngine::~MfSourcePlaybackEngine()
    {
        uninit();
        DeleteCriticalSection(&m_waveLock);
        DeleteCriticalSection(&m_frameLock);
        DeleteCriticalSection(&m_readerLock);
        DeleteCriticalSection(&m_stateLock);
    }

    HRESULT MfSourcePlaybackEngine::init(const PlaybackEngineInitParam& param)
    {
        uninit();

        if (param.m_filePath.empty())
        {
            m_lastError = L"播放文件路径为空。";
            return E_INVALIDARG;
        }

        Logf(L"MfSourcePlaybackEngine init begin: path=%s, videoWindow=%p, eventWindow=%p, eventMessage=%u, timelineValid=%d, first=%s (%I64d), lastEnd=%s (%I64d), sourceDuration=%s (%I64d), requestedOffset=%s (%I64d)",
             param.m_filePath.c_str(),
             param.m_videoWindow,
             param.m_eventWindow,
             param.m_eventMessage,
             param.m_videoTimeline.isValid ? 1 : 0,
             FormatTime(param.m_videoTimeline.firstTime).c_str(),
             param.m_videoTimeline.firstTime,
             FormatTime(param.m_videoTimeline.lastEndTime).c_str(),
             param.m_videoTimeline.lastEndTime,
             FormatTime(param.m_videoTimeline.sourceDuration).c_str(),
             param.m_videoTimeline.sourceDuration,
             FormatTime(param.m_videoTimelineOffset).c_str(),
             param.m_videoTimelineOffset);

        m_filePath = param.m_filePath;
        m_videoWindow = param.m_videoWindow;
        m_eventWindow = param.m_eventWindow;
        m_eventMessage = param.m_eventMessage;
        m_videoTimelineOffset = param.m_videoTimelineOffset > 0 ? param.m_videoTimelineOffset : 0;
        m_frameDuration = param.m_fallbackFrameDuration > 0 ? param.m_fallbackFrameDuration : DEFAULT_FRAME_DURATION;
        m_duration = param.m_videoTimeline.sourceDuration > 0 ? param.m_videoTimeline.sourceDuration : 0;
        m_lastError.clear();
        m_videoSampleLogCount = 0;
        m_audioSampleLogCount = 0;
        m_videoReorderLogCount = 0;
        m_lastPublishedVideoTime = -1;
        m_videoEnded = false;
        m_audioEnded = false;
        m_videoSourceEnded = false;
        m_seekSerial = 0;
        m_videoReorderFrames.clear();

        m_exitEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_wakeupEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_exitEvent || !m_wakeupEvent)
        {
            const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            m_lastError = HResultText(hr);
            Logf(L"Create playback events failed: 0x%08X", static_cast<unsigned int>(hr));
            uninit();
            return hr;
        }

        HRESULT hr = openVideoReader();
        if (FAILED(hr))
        {
            Logf(L"Open video reader failed: 0x%08X", static_cast<unsigned int>(hr));
        }

        HRESULT audioHr = openAudioReader();
        if (FAILED(audioHr))
        {
            Logf(L"Open audio reader failed or no audio stream: 0x%08X", static_cast<unsigned int>(audioHr));
        }

        if (!m_videoAvailable && !m_audioAvailable)
        {
            hr = FAILED(hr) ? hr : audioHr;
            if (SUCCEEDED(hr))
            {
                hr = MF_E_INVALIDMEDIATYPE;
            }
            m_lastError = L"没有找到可播放的视频或音频流。";
            Logf(L"No playable streams. result=0x%08X", static_cast<unsigned int>(hr));
            uninit();
            return hr;
        }

        if (m_videoAvailable && param.m_videoTimeline.isValid)
        {
            REFERENCE_TIME logicalVideoEnd = param.m_videoTimeline.lastEndTime - m_videoTimelineOffset;
            if (logicalVideoEnd > m_duration)
            {
                Logf(L"Duration expanded by logical video timeline: old=%s (%I64d), logicalVideoEnd=%s (%I64d)",
                     FormatTime(m_duration).c_str(),
                     m_duration,
                     FormatTime(logicalVideoEnd).c_str(),
                     logicalVideoEnd);
                m_duration = logicalVideoEnd;
            }
        }

        if (m_duration <= 0)
        {
            m_duration = DEFAULT_FRAME_DURATION;
            Logf(L"Duration fallback applied: %s (%I64d)", FormatTime(m_duration).c_str(), m_duration);
        }

        hr = startThreads();
        if (FAILED(hr))
        {
            uninit();
            return hr;
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            m_isInit = true;
            m_state = PlaybackEngineStateStopped;
            m_lastResult = S_OK;
            resetClockLocked(0);
        }

        previewVideoFrameAt(0, kVideoPreviewMaxReadCount);
        Logf(L"MfSourcePlaybackEngine init end: video=%d audio=%d native=%dx%d stride=%ld duration=%s (%I64d), frameDuration=%s (%I64d), offset=%s (%I64d)",
             m_videoAvailable ? 1 : 0,
             m_audioAvailable ? 1 : 0,
             m_videoWidth,
             m_videoHeight,
             m_videoStride,
             FormatTime(m_duration).c_str(),
             m_duration,
             FormatTime(m_frameDuration).c_str(),
             m_frameDuration,
             FormatTime(m_videoTimelineOffset).c_str(),
             m_videoTimelineOffset);
        return S_OK;
    }

    void MfSourcePlaybackEngine::uninit()
    {
        const bool needLog = m_isInit || m_videoReader || m_audioReader || m_waveOut || m_videoThread || m_audioThread;
        if (needLog)
        {
            Logf(L"MfSourcePlaybackEngine uninit begin");
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            m_isExit = true;
            m_state = PlaybackEngineStateClosed;
        }

        if (m_exitEvent)
        {
            SetEvent(m_exitEvent);
        }
        if (m_wakeupEvent)
        {
            SetEvent(m_wakeupEvent);
        }

        stopThreads();
        closeWaveOut();

        {
            AutoCriticalSectionLock lock(&m_readerLock);
            clearVideoReorderFramesLocked();
            SafeRelease(m_videoReader);
            SafeRelease(m_audioReader);
        }

        if (m_wakeupEvent)
        {
            CloseHandle(m_wakeupEvent);
            m_wakeupEvent = nullptr;
        }
        if (m_exitEvent)
        {
            CloseHandle(m_exitEvent);
            m_exitEvent = nullptr;
        }

        {
            AutoCriticalSectionLock frameLock(&m_frameLock);
            m_currentFrame = PlaybackVideoFrame();
            ZeroMemory(&m_videoBitmapInfo, sizeof(m_videoBitmapInfo));
        }

        m_filePath.clear();
        m_lastError.clear();
        m_videoWindow = nullptr;
        m_eventWindow = nullptr;
        m_eventMessage = 0;
        m_videoAvailable = false;
        m_audioAvailable = false;
        m_videoEnded = false;
        m_audioEnded = false;
        m_videoSourceEnded = false;
        m_isInit = false;
        m_isExit = false;
        m_seekSerial = 0;
        m_duration = 0;
        m_clockBasePosition = 0;
        m_frameDuration = DEFAULT_FRAME_DURATION;
        m_videoTimelineOffset = 0;
        m_playbackRate = 1.0;
        m_videoWidth = 0;
        m_videoHeight = 0;
        m_videoStride = 0;
        m_nextAudioBuffer = 0;
        m_videoReorderLogCount = 0;
        m_lastPublishedVideoTime = -1;
        m_audioBuffers.clear();
        m_videoReorderFrames.clear();
        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));

        if (needLog)
        {
            Logf(L"MfSourcePlaybackEngine uninit end");
        }
    }

    HRESULT MfSourcePlaybackEngine::play()
    {
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            if (!m_isInit)
            {
                return MF_E_NOT_INITIALIZED;
            }

            const REFERENCE_TIME current = calculatePositionLocked();
            if (m_state == PlaybackEngineStateEnded || current >= m_duration)
            {
                resetClockLocked(0);
                m_videoEnded = false;
                m_audioEnded = false;
            }
            else
            {
                resetClockLocked(current);
            }

            m_state = PlaybackEngineStatePlaying;
            m_lastResult = S_OK;
            Logf(L"Engine play: position=%s (%I64d), duration=%s (%I64d), rate=%.3f",
                 FormatTime(m_clockBasePosition).c_str(),
                 m_clockBasePosition,
                 FormatTime(m_duration).c_str(),
                 m_duration,
                 m_playbackRate);
        }

        {
            AutoCriticalSectionLock waveLock(&m_waveLock);
            if (m_waveOut)
            {
                MMRESULT mm = waveOutRestart(m_waveOut);
                Logf(L"waveOutRestart returned %u", static_cast<unsigned int>(mm));
            }
        }

        if (m_wakeupEvent)
        {
            SetEvent(m_wakeupEvent);
        }
        return S_OK;
    }

    HRESULT MfSourcePlaybackEngine::pause()
    {
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            if (!m_isInit)
            {
                return MF_E_NOT_INITIALIZED;
            }

            const REFERENCE_TIME current = calculatePositionLocked();
            resetClockLocked(current);
            m_state = PlaybackEngineStatePaused;
            m_lastResult = S_OK;
            Logf(L"Engine pause: position=%s (%I64d)", FormatTime(m_clockBasePosition).c_str(), m_clockBasePosition);
        }

        {
            AutoCriticalSectionLock waveLock(&m_waveLock);
            if (m_waveOut)
            {
                MMRESULT mm = waveOutPause(m_waveOut);
                Logf(L"waveOutPause returned %u", static_cast<unsigned int>(mm));
            }
        }
        return S_OK;
    }

    HRESULT MfSourcePlaybackEngine::stop()
    {
        if (!m_isInit)
        {
            return MF_E_NOT_INITIALIZED;
        }

        Logf(L"Engine stop requested");
        HRESULT hr = seek(0, false);
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            resetClockLocked(0);
            m_state = PlaybackEngineStateStopped;
            m_lastResult = hr;
        }
        return hr;
    }

    HRESULT MfSourcePlaybackEngine::seek(REFERENCE_TIME position, bool resumeIfPreviousPlaying, size_t previewMaxReadCount)
    {
        if (!m_isInit)
        {
            return MF_E_NOT_INITIALIZED;
        }

        if (position < 0)
        {
            position = 0;
        }
        if (m_duration > 0 && position > m_duration)
        {
            position = m_duration;
        }

        PlaybackEngineState previousState = PlaybackEngineStateClosed;
        DWORD seekSerial = 0;
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            previousState = m_state;
            resetClockLocked(position);
            m_videoEnded = false;
            m_audioEnded = false;
            m_state = PlaybackEngineStatePaused;
            ++m_seekSerial;
            seekSerial = m_seekSerial;
        }

        {
            AutoCriticalSectionLock waveLock(&m_waveLock);
            resetWaveBuffers();
        }

        const size_t actualPreviewMaxReadCount = previewMaxReadCount > 0 ? previewMaxReadCount : kVideoPreviewMaxReadCount;
        REFERENCE_TIME videoPreviewPreroll = 0;
        if (m_frameDuration > 0 && actualPreviewMaxReadCount > kVideoReorderFrameCount)
        {
            videoPreviewPreroll = m_frameDuration * static_cast<REFERENCE_TIME>(actualPreviewMaxReadCount - kVideoReorderFrameCount);
        }

        const REFERENCE_TIME rawVideoTarget = position + m_videoTimelineOffset;
        REFERENCE_TIME videoSeekTime = rawVideoTarget > videoPreviewPreroll ? rawVideoTarget - videoPreviewPreroll : 0;
        if (m_duration > m_frameDuration && videoSeekTime > m_duration - m_frameDuration)
        {
            videoSeekTime = m_duration - m_frameDuration;
        }
        if (videoSeekTime < 0)
        {
            videoSeekTime = 0;
        }

        PROPVARIANT videoPosition;
        PROPVARIANT audioPosition;
        InitInt64PropVariant(videoPosition, videoSeekTime);
        InitInt64PropVariant(audioPosition, position);

        HRESULT videoHr = S_OK;
        HRESULT audioHr = S_OK;
        {
            AutoCriticalSectionLock readerLock(&m_readerLock);
            if (m_videoReader)
            {
                videoHr = m_videoReader->SetCurrentPosition(GUID_NULL, videoPosition);
            }
            if (m_audioReader)
            {
                audioHr = m_audioReader->SetCurrentPosition(GUID_NULL, audioPosition);
            }
            clearVideoReorderFramesLocked();
        }

        {
            AutoCriticalSectionLock frameLock(&m_frameLock);
            m_lastPublishedVideoTime = -1;
        }

        PropVariantClear(&videoPosition);
        PropVariantClear(&audioPosition);

        Logf(L"Engine seek: logical=%s (%I64d), rawVideoTarget=%s (%I64d), videoSeek=%s (%I64d), videoPreroll=%s (%I64d), videoTimestampOffset=%s (%I64d), audioSeek=%s (%I64d), videoHr=0x%08X, audioHr=0x%08X, previousState=%s, resumePrevious=%d, previewMax=%u, seekSerial=%lu",
             FormatTime(position).c_str(),
             position,
             FormatTime(rawVideoTarget).c_str(),
             rawVideoTarget,
             FormatTime(videoSeekTime).c_str(),
             videoSeekTime,
             FormatTime(videoPreviewPreroll).c_str(),
             videoPreviewPreroll,
             FormatTime(m_videoTimelineOffset).c_str(),
             m_videoTimelineOffset,
             FormatTime(position).c_str(),
             position,
             static_cast<unsigned int>(videoHr),
             static_cast<unsigned int>(audioHr),
             PlaybackEngineStateName(previousState),
             resumeIfPreviousPlaying ? 1 : 0,
             static_cast<unsigned int>(actualPreviewMaxReadCount),
             seekSerial);

        HRESULT previewHr = previewVideoFrameAt(position, actualPreviewMaxReadCount);
        if (FAILED(videoHr))
        {
            setStateLocked(PlaybackEngineStateError, videoHr);
            postEvent(PlaybackEngineEventError, videoHr);
            return videoHr;
        }
        if (FAILED(audioHr))
        {
            Logf(L"Audio seek failed but video path can continue: 0x%08X", static_cast<unsigned int>(audioHr));
        }
        if (FAILED(previewHr))
        {
            Logf(L"Video preview after seek failed: 0x%08X", static_cast<unsigned int>(previewHr));
        }

        if (previousState == PlaybackEngineStatePlaying && resumeIfPreviousPlaying)
        {
            play();
        }
        else
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            m_state = previousState == PlaybackEngineStateStopped ? PlaybackEngineStateStopped : PlaybackEngineStatePaused;
        }

        postEvent(PlaybackEngineEventPositionSet, S_OK);
        return S_OK;
    }

    HRESULT MfSourcePlaybackEngine::setRate(double rate)
    {
        if (rate < 0.1)
        {
            rate = 0.1;
        }
        if (rate > 3.0)
        {
            rate = 3.0;
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            const REFERENCE_TIME current = calculatePositionLocked();
            resetClockLocked(current);
            m_playbackRate = rate;
            m_lastResult = S_OK;
        }

        MMRESULT mm = MMSYSERR_NOERROR;
        {
            AutoCriticalSectionLock waveLock(&m_waveLock);
            if (m_waveOut)
            {
                const DWORD fixedRate = static_cast<DWORD>(rate * 65536.0 + 0.5);
                mm = waveOutSetPlaybackRate(m_waveOut, fixedRate);
            }
        }

        Logf(L"Engine setRate: rate=%.3f, waveOutSetPlaybackRate=%u", rate, static_cast<unsigned int>(mm));
        return S_OK;
    }

    bool MfSourcePlaybackEngine::getPosition(REFERENCE_TIME& position)
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        if (!m_isInit)
        {
            position = 0;
            return false;
        }

        position = calculatePositionLocked();
        return true;
    }

    void MfSourcePlaybackEngine::snapshot(PlaybackEngineSnapshot& outSnapshot)
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        outSnapshot.m_state = m_state;
        outSnapshot.m_lastResult = m_lastResult;
        outSnapshot.m_position = calculatePositionLocked();
        outSnapshot.m_duration = m_duration;
        outSnapshot.m_frameDuration = m_frameDuration;
        outSnapshot.m_playbackRate = m_playbackRate;
        outSnapshot.m_videoAvailable = m_videoAvailable;
        outSnapshot.m_audioAvailable = m_audioAvailable;
        outSnapshot.m_videoEnded = m_videoEnded;
        outSnapshot.m_audioEnded = m_audioEnded;
    }

    bool MfSourcePlaybackEngine::isOpen() const
    {
        return m_isInit;
    }

    REFERENCE_TIME MfSourcePlaybackEngine::duration() const
    {
        return m_duration;
    }

    REFERENCE_TIME MfSourcePlaybackEngine::frameDuration() const
    {
        return m_frameDuration;
    }

    bool MfSourcePlaybackEngine::getNativeVideoSize(int& width, int& height) const
    {
        width = m_videoWidth;
        height = m_videoHeight;
        return width > 0 && height > 0;
    }

    PlaybackEngineState MfSourcePlaybackEngine::state()
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        return m_state;
    }

    std::wstring MfSourcePlaybackEngine::lastError() const
    {
        return m_lastError;
    }

    void MfSourcePlaybackEngine::paintVideo(HDC hdc, const RECT& rect)
    {
        if (!hdc)
        {
            return;
        }

        AutoCriticalSectionLock lock(&m_frameLock);
        if (m_currentFrame.m_pixels.empty() || m_currentFrame.m_width <= 0 || m_currentFrame.m_height <= 0)
        {
            FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            return;
        }

        // 视频缩放时使用HALFTONE，避免默认拉伸造成明显锯齿和彩色边缘。
        POINT oldBrushOrg = { 0 };
        const int oldStretchMode = SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, 0, 0, &oldBrushOrg);
        StretchDIBits(hdc,
                      rect.left,
                      rect.top,
                      max(1, rect.right - rect.left),
                      max(1, rect.bottom - rect.top),
                      0,
                      0,
                      m_currentFrame.m_width,
                      m_currentFrame.m_height,
                      &m_currentFrame.m_pixels[0],
                      &m_videoBitmapInfo,
                      DIB_RGB_COLORS,
                      SRCCOPY);
        SetBrushOrgEx(hdc, oldBrushOrg.x, oldBrushOrg.y, nullptr);
        if (oldStretchMode != 0)
        {
            SetStretchBltMode(hdc, oldStretchMode);
        }
    }

    HRESULT MfSourcePlaybackEngine::createReaderAttributes(IMFAttributes** attributes)
    {
        if (!attributes)
        {
            return E_POINTER;
        }

        *attributes = nullptr;
        HRESULT hr = MFCreateAttributes(attributes, 3);
        if (FAILED(hr))
        {
            return hr;
        }

        // 问题视频在Windows软件H.264解码器里出现过访问异常，这里保留系统默认的硬件变换路径。
        (*attributes)->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        (*attributes)->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        Logf(L"SourceReader attributes: hardwareTransforms=1, videoProcessing=1");
        return S_OK;
    }

    HRESULT MfSourcePlaybackEngine::openVideoReader()
    {
        IMFAttributes* attributes = nullptr;
        HRESULT hr = createReaderAttributes(&attributes);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = MFCreateSourceReaderFromURL(m_filePath.c_str(), attributes, &m_videoReader);
        SafeRelease(attributes);
        if (FAILED(hr))
        {
            return hr;
        }
        Logf(L"Video SourceReader created");

        hr = m_videoReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
        Logf(L"Video reader deselect all streams returned 0x%08X", static_cast<unsigned int>(hr));

        hr = m_videoReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        if (FAILED(hr))
        {
            return hr;
        }

        IMFMediaType* videoType = nullptr;
        hr = MFCreateMediaType(&videoType);
        if (FAILED(hr))
        {
            return hr;
        }
        videoType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        videoType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        videoType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

        hr = m_videoReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, videoType);
        SafeRelease(videoType);
        if (FAILED(hr))
        {
            return hr;
        }

        IMFMediaType* currentType = nullptr;
        hr = m_videoReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &currentType);
        if (SUCCEEDED(hr) && currentType)
        {
            readVideoTypeInfo(currentType);
        }
        SafeRelease(currentType);

        readSourceDuration(m_videoReader);
        m_videoAvailable = m_videoWidth > 0 && m_videoHeight > 0;
        Logf(L"Video reader open end: result=0x%08X, available=%d, native=%dx%d, stride=%ld",
             static_cast<unsigned int>(hr),
             m_videoAvailable ? 1 : 0,
             m_videoWidth,
             m_videoHeight,
             m_videoStride);
        return hr;
    }

    HRESULT MfSourcePlaybackEngine::openAudioReader()
    {
        IMFAttributes* attributes = nullptr;
        HRESULT hr = createReaderAttributes(&attributes);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = MFCreateSourceReaderFromURL(m_filePath.c_str(), attributes, &m_audioReader);
        SafeRelease(attributes);
        if (FAILED(hr))
        {
            return hr;
        }
        Logf(L"Audio SourceReader created");

        hr = m_audioReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
        Logf(L"Audio reader deselect all streams returned 0x%08X", static_cast<unsigned int>(hr));

        hr = m_audioReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        if (FAILED(hr))
        {
            SafeRelease(m_audioReader);
            return hr;
        }

        IMFMediaType* audioType = nullptr;
        hr = MFCreateMediaType(&audioType);
        if (FAILED(hr))
        {
            return hr;
        }
        audioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        audioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        audioType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);

        hr = m_audioReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioType);
        SafeRelease(audioType);
        if (FAILED(hr))
        {
            SafeRelease(m_audioReader);
            return hr;
        }

        IMFMediaType* currentType = nullptr;
        hr = m_audioReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &currentType);
        if (SUCCEEDED(hr) && currentType)
        {
            hr = openWaveOut(currentType);
        }
        SafeRelease(currentType);

        if (FAILED(hr))
        {
            SafeRelease(m_audioReader);
            return hr;
        }

        readSourceDuration(m_audioReader);
        m_audioAvailable = true;
        m_audioBuffers.resize(kAudioBufferCount);
        Logf(L"Audio reader open end: available=%d, channels=%u, samplesPerSec=%u, bits=%u, blockAlign=%u, avgBytes=%u",
             m_audioAvailable ? 1 : 0,
             static_cast<unsigned int>(m_waveFormat.nChannels),
             static_cast<unsigned int>(m_waveFormat.nSamplesPerSec),
             static_cast<unsigned int>(m_waveFormat.wBitsPerSample),
             static_cast<unsigned int>(m_waveFormat.nBlockAlign),
             static_cast<unsigned int>(m_waveFormat.nAvgBytesPerSec));
        return S_OK;
    }

    void MfSourcePlaybackEngine::readSourceDuration(IMFSourceReader* reader)
    {
        if (!reader)
        {
            return;
        }

        PROPVARIANT value;
        PropVariantInit(&value);
        HRESULT hr = reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &value);
        LONGLONG duration = 0;
        if (SUCCEEDED(hr) && ReadInt64PropVariant(value, duration) && duration > m_duration)
        {
            m_duration = duration;
        }
        Logf(L"Source duration read: hr=0x%08X, vt=%u, duration=%s (%I64d), stored=%s (%I64d)",
             static_cast<unsigned int>(hr),
             static_cast<unsigned int>(value.vt),
             FormatTime(duration).c_str(),
             duration,
             FormatTime(m_duration).c_str(),
             m_duration);
        PropVariantClear(&value);
    }

    void MfSourcePlaybackEngine::readVideoTypeInfo(IMFMediaType* mediaType)
    {
        if (!mediaType)
        {
            return;
        }

        UINT32 width = 0;
        UINT32 height = 0;
        HRESULT sizeHr = MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height);
        if (SUCCEEDED(sizeHr))
        {
            m_videoWidth = static_cast<int>(width);
            m_videoHeight = static_cast<int>(height);
        }

        UINT32 frameRateNum = 0;
        UINT32 frameRateDen = 0;
        HRESULT frameRateHr = MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &frameRateNum, &frameRateDen);
        if (SUCCEEDED(frameRateHr) && frameRateNum > 0 && frameRateDen > 0)
        {
            m_frameDuration = static_cast<REFERENCE_TIME>(static_cast<double>(ONE_SECOND) * frameRateDen / frameRateNum + 0.5);
        }

        UINT32 strideValue = 0;
        HRESULT strideHr = mediaType->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideValue);
        if (SUCCEEDED(strideHr))
        {
            m_videoStride = static_cast<LONG>(strideValue);
        }
        else if (m_videoWidth > 0)
        {
            LONG stride = 0;
            HRESULT calcHr = MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, m_videoWidth, &stride);
            if (SUCCEEDED(calcHr))
            {
                m_videoStride = stride;
            }
            else
            {
                m_videoStride = m_videoWidth * 4;
            }
        }

        Logf(L"Video type: sizeHr=0x%08X, size=%ux%u, frameRateHr=0x%08X, fps=%u/%u, frameDuration=%s (%I64d), strideHr=0x%08X, stride=%ld",
             static_cast<unsigned int>(sizeHr),
             width,
             height,
             static_cast<unsigned int>(frameRateHr),
             frameRateNum,
             frameRateDen,
             FormatTime(m_frameDuration).c_str(),
             m_frameDuration,
             static_cast<unsigned int>(strideHr),
             m_videoStride);
    }

    HRESULT MfSourcePlaybackEngine::openWaveOut(IMFMediaType* mediaType)
    {
        if (!mediaType)
        {
            return E_POINTER;
        }

        UINT32 channels = 0;
        UINT32 samplesPerSecond = 0;
        UINT32 bitsPerSample = 0;
        UINT32 blockAlign = 0;
        UINT32 avgBytesPerSecond = 0;

        mediaType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
        mediaType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSecond);
        mediaType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
        mediaType->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &blockAlign);
        mediaType->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &avgBytesPerSecond);

        if (channels == 0 || samplesPerSecond == 0 || bitsPerSample == 0)
        {
            Logf(L"Invalid audio type: channels=%u, samples=%u, bits=%u", channels, samplesPerSecond, bitsPerSample);
            return MF_E_INVALIDMEDIATYPE;
        }

        if (blockAlign == 0)
        {
            blockAlign = channels * bitsPerSample / 8;
        }
        if (avgBytesPerSecond == 0)
        {
            avgBytesPerSecond = samplesPerSecond * blockAlign;
        }

        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));
        m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        m_waveFormat.nChannels = static_cast<WORD>(channels);
        m_waveFormat.nSamplesPerSec = samplesPerSecond;
        m_waveFormat.wBitsPerSample = static_cast<WORD>(bitsPerSample);
        m_waveFormat.nBlockAlign = static_cast<WORD>(blockAlign);
        m_waveFormat.nAvgBytesPerSec = avgBytesPerSecond;
        m_waveFormat.cbSize = 0;

        MMRESULT mm = waveOutOpen(&m_waveOut, WAVE_MAPPER, &m_waveFormat, 0, 0, CALLBACK_NULL);
        Logf(L"waveOutOpen returned %u", static_cast<unsigned int>(mm));
        if (mm != MMSYSERR_NOERROR)
        {
            m_waveOut = nullptr;
            return HRESULT_FROM_WIN32(mm);
        }

        waveOutPause(m_waveOut);
        return S_OK;
    }

    HRESULT MfSourcePlaybackEngine::startThreads()
    {
        DWORD threadId = 0;
        if (m_videoAvailable)
        {
            m_videoThread = CreateThread(nullptr, 0, MfSourcePlaybackEngine::videoThreadProc, this, 0, &threadId);
            if (!m_videoThread)
            {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                Logf(L"Create video thread failed: 0x%08X", static_cast<unsigned int>(hr));
                return hr;
            }
            Logf(L"Video thread created: threadId=%lu", threadId);
        }

        if (m_audioAvailable)
        {
            m_audioThread = CreateThread(nullptr, 0, MfSourcePlaybackEngine::audioThreadProc, this, 0, &threadId);
            if (!m_audioThread)
            {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                Logf(L"Create audio thread failed: 0x%08X", static_cast<unsigned int>(hr));
                return hr;
            }
            Logf(L"Audio thread created: threadId=%lu", threadId);
        }

        return S_OK;
    }

    void MfSourcePlaybackEngine::stopThreads()
    {
        HANDLE handles[2] = { nullptr, nullptr };
        DWORD count = 0;
        if (m_videoThread)
        {
            handles[count++] = m_videoThread;
        }
        if (m_audioThread)
        {
            handles[count++] = m_audioThread;
        }

        if (count > 0)
        {
            WaitForMultipleObjects(count, handles, TRUE, 5000);
        }

        if (m_videoThread)
        {
            CloseHandle(m_videoThread);
            m_videoThread = nullptr;
        }
        if (m_audioThread)
        {
            CloseHandle(m_audioThread);
            m_audioThread = nullptr;
        }
    }

    DWORD WINAPI MfSourcePlaybackEngine::videoThreadProc(LPVOID param)
    {
        MfSourcePlaybackEngine* self = reinterpret_cast<MfSourcePlaybackEngine*>(param);
        if (self)
        {
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            self->videoThreadMain();
            CoUninitialize();
        }
        return 0;
    }

    DWORD WINAPI MfSourcePlaybackEngine::audioThreadProc(LPVOID param)
    {
        MfSourcePlaybackEngine* self = reinterpret_cast<MfSourcePlaybackEngine*>(param);
        if (self)
        {
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            self->audioThreadMain();
            CoUninitialize();
        }
        return 0;
    }

    void MfSourcePlaybackEngine::videoThreadMain()
    {
        Logf(L"Video thread enter");
        while (!shouldExit())
        {
            if (!waitForPlayableState())
            {
                break;
            }

            const DWORD frameSeekSerial = readSeekSerial();
            PlaybackVideoFrame frame;
            bool endOfStream = false;
            HRESULT hr = readNextVideoFrame(frame, endOfStream);
            if (FAILED(hr))
            {
                setStateLocked(PlaybackEngineStateError, hr);
                postEvent(PlaybackEngineEventError, hr);
                break;
            }
            if (endOfStream)
            {
                bool shouldMarkVideoEnded = false;
                {
                    AutoCriticalSectionLock lock(&m_stateLock);
                    shouldMarkVideoEnded = !m_videoEnded;
                }
                if (shouldMarkVideoEnded)
                {
                    markStreamEnded(true, false);
                }
                Sleep(kThreadWaitMs);
                continue;
            }

            if (frameSeekSerial != readSeekSerial())
            {
                Logf(L"Video frame discarded after read because seek changed: frame=%s (%I64d), oldSerial=%lu, currentSerial=%lu",
                     FormatTime(frame.m_time).c_str(),
                     frame.m_time,
                     frameSeekSerial,
                     readSeekSerial());
                continue;
            }

            bool skipFrame = false;
            for (;;)
            {
                if (shouldExit())
                {
                    Logf(L"Video thread exit while waiting frame clock");
                    return;
                }

                if (frameSeekSerial != readSeekSerial())
                {
                    skipFrame = true;
                    Logf(L"Video frame skipped because seek changed: frame=%s (%I64d), oldSerial=%lu, currentSerial=%lu",
                         FormatTime(frame.m_time).c_str(),
                         frame.m_time,
                         frameSeekSerial,
                         readSeekSerial());
                    break;
                }

                PlaybackEngineState currentState = state();
                if (currentState != PlaybackEngineStatePlaying)
                {
                    skipFrame = true;
                    break;
                }

                REFERENCE_TIME pos = 0;
                getPosition(pos);
                if (frame.m_time + frame.m_duration < pos - m_frameDuration)
                {
                    skipFrame = true;
                    break;
                }
                if (pos + m_frameDuration / 3 >= frame.m_time)
                {
                    break;
                }
                Sleep(kVideoWaitSliceMs);
            }

            if (!skipFrame)
            {
                if (m_videoSampleLogCount <= 10 || (m_videoSampleLogCount % 300) == 0)
                {
                    Logf(L"Video frame publish from thread: time=%s (%I64d), duration=%s (%I64d), seekSerial=%lu",
                         FormatTime(frame.m_time).c_str(),
                         frame.m_time,
                         FormatTime(frame.m_duration).c_str(),
                         frame.m_duration,
                         frameSeekSerial);
                }
                publishVideoFrame(frame);
            }
        }
        Logf(L"Video thread leave");
    }

    void MfSourcePlaybackEngine::audioThreadMain()
    {
        Logf(L"Audio thread enter");
        while (!shouldExit())
        {
            if (!waitForPlayableState())
            {
                break;
            }

            bool endOfStream = false;
            HRESULT hr = readAndQueueAudio(endOfStream);
            if (FAILED(hr))
            {
                setStateLocked(PlaybackEngineStateError, hr);
                postEvent(PlaybackEngineEventError, hr);
                break;
            }
            if (endOfStream)
            {
                bool shouldMarkAudioEnded = false;
                {
                    AutoCriticalSectionLock lock(&m_stateLock);
                    shouldMarkAudioEnded = !m_audioEnded;
                }
                if (shouldMarkAudioEnded)
                {
                    markStreamEnded(false, true);
                }
                Sleep(kThreadWaitMs);
                continue;
            }
        }
        Logf(L"Audio thread leave");
    }

    bool MfSourcePlaybackEngine::waitForPlayableState()
    {
        for (;;)
        {
            if (shouldExit())
            {
                return false;
            }

            PlaybackEngineState currentState = state();
            if (currentState == PlaybackEngineStatePlaying)
            {
                return true;
            }

            WaitForSingleObject(m_wakeupEvent, kThreadWaitMs);
            ResetEvent(m_wakeupEvent);
        }
    }

    HRESULT MfSourcePlaybackEngine::readNextVideoFrame(PlaybackVideoFrame& frame, bool& endOfStream)
    {
        endOfStream = false;
        if (!m_videoReader)
        {
            endOfStream = true;
            return S_OK;
        }

        AutoCriticalSectionLock readerLock(&m_readerLock);
        HRESULT hr = fillVideoReorderFramesLocked(kVideoReorderFrameCount);
        if (FAILED(hr))
        {
            return hr;
        }

        if (popNextVideoFrameLocked(frame))
        {
            return S_OK;
        }

        endOfStream = m_videoSourceEnded;
        if (endOfStream)
        {
            Logf(L"Video reorder queue drained after source end");
        }
        return S_OK;
    }

    HRESULT MfSourcePlaybackEngine::readNextVideoFrameFromSourceLocked(PlaybackVideoFrame& frame, bool& endOfStream, REFERENCE_TIME copyPixelsFromTime)
    {
        endOfStream = false;
        if (!m_videoReader)
        {
            endOfStream = true;
            return S_OK;
        }

        for (;;)
        {
            DWORD actualStream = 0;
            DWORD flags = 0;
            LONGLONG timestamp = 0;
            IMFSample* sample = nullptr;
            HRESULT hr = m_videoReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                   0,
                                                   &actualStream,
                                                   &flags,
                                                   &timestamp,
                                                   &sample);

            if (FAILED(hr))
            {
                SafeRelease(sample);
                Logf(L"Video ReadSample failed: hr=0x%08X", static_cast<unsigned int>(hr));
                return hr;
            }

            if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
            {
                Logf(L"Video media type changed");
            }
            if ((flags & MF_SOURCE_READERF_ERROR) != 0)
            {
                SafeRelease(sample);
                Logf(L"Video ReadSample returned error flag: flags=0x%08X", flags);
                return E_FAIL;
            }
            if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
            {
                SafeRelease(sample);
                endOfStream = true;
                Logf(L"Video end of stream reached");
                return S_OK;
            }
            if (!sample)
            {
                continue;
            }

            LONGLONG sampleTime = timestamp;
            LONGLONG explicitSampleTime = 0;
            if (SUCCEEDED(sample->GetSampleTime(&explicitSampleTime)))
            {
                sampleTime = explicitSampleTime;
            }

            LONGLONG sampleDuration = m_frameDuration;
            if (FAILED(sample->GetSampleDuration(&sampleDuration)) || sampleDuration <= 0)
            {
                sampleDuration = m_frameDuration;
            }

            REFERENCE_TIME logicalTime = sampleTime - m_videoTimelineOffset;
            if (logicalTime < 0)
            {
                logicalTime = 0;
            }

            HRESULT copyHr = S_OK;
            if (copyPixelsFromTime < 0 || logicalTime >= copyPixelsFromTime)
            {
                copyHr = copyVideoSample(sample, logicalTime, sampleDuration, frame);
            }
            else
            {
                frame.m_width = m_videoWidth;
                frame.m_height = m_videoHeight;
                frame.m_stride = m_videoWidth * 4;
                frame.m_time = logicalTime;
                frame.m_duration = sampleDuration > 0 ? sampleDuration : m_frameDuration;
            }
            SafeRelease(sample);
            if (FAILED(copyHr))
            {
                return copyHr;
            }

            ++m_videoSampleLogCount;
            if (m_videoSampleLogCount <= 5 || (m_videoSampleLogCount % 300) == 0)
            {
                Logf(L"Video sample decoded: index=%lu, raw=%s (%I64d), logical=%s (%I64d), duration=%s (%I64d), bytes=%u",
                     m_videoSampleLogCount,
                     FormatTime(sampleTime).c_str(),
                     sampleTime,
                     FormatTime(logicalTime).c_str(),
                     logicalTime,
                     FormatTime(sampleDuration).c_str(),
                     sampleDuration,
                     static_cast<unsigned int>(frame.m_pixels.size()));
            }
            return S_OK;
        }
    }

    HRESULT MfSourcePlaybackEngine::fillVideoReorderFramesLocked(size_t minCount)
    {
        while (!m_videoSourceEnded && m_videoReorderFrames.size() < minCount)
        {
            PlaybackVideoFrame frame;
            bool endOfStream = false;
            HRESULT hr = readNextVideoFrameFromSourceLocked(frame, endOfStream);
            if (FAILED(hr))
            {
                return hr;
            }
            if (endOfStream)
            {
                m_videoSourceEnded = true;
                break;
            }

            if (!m_videoReorderFrames.empty() && frame.m_time < m_videoReorderFrames.back().m_time && m_videoReorderLogCount < 20)
            {
                ++m_videoReorderLogCount;
                Logf(L"Video reorder input detected: previous=%s (%I64d), current=%s (%I64d), queue=%u",
                     FormatTime(m_videoReorderFrames.back().m_time).c_str(),
                     m_videoReorderFrames.back().m_time,
                     FormatTime(frame.m_time).c_str(),
                     frame.m_time,
                     static_cast<unsigned int>(m_videoReorderFrames.size()));
            }
            m_videoReorderFrames.push_back(frame);
        }
        return S_OK;
    }

    bool MfSourcePlaybackEngine::popNextVideoFrameLocked(PlaybackVideoFrame& frame)
    {
        if (m_videoReorderFrames.empty())
        {
            return false;
        }

        std::stable_sort(m_videoReorderFrames.begin(), m_videoReorderFrames.end(), VideoFrameTimeLess);
        frame = m_videoReorderFrames.front();
        m_videoReorderFrames.erase(m_videoReorderFrames.begin());
        return true;
    }

    void MfSourcePlaybackEngine::clearVideoReorderFramesLocked()
    {
        if (!m_videoReorderFrames.empty())
        {
            Logf(L"Video reorder queue cleared: frames=%u", static_cast<unsigned int>(m_videoReorderFrames.size()));
        }
        m_videoReorderFrames.clear();
        m_videoSourceEnded = false;
    }

    HRESULT MfSourcePlaybackEngine::previewVideoFrameAt(REFERENCE_TIME position, size_t maxReadCount)
    {
        if (!m_videoReader)
        {
            return S_OK;
        }

        if (maxReadCount == 0)
        {
            maxReadCount = kVideoPreviewMaxReadCount;
        }

        PlaybackVideoFrame frame;
        bool hasFrame = false;
        bool endOfStream = false;
        size_t readCount = 0;
        size_t queueCount = 0;
        HRESULT hr = S_OK;
        const REFERENCE_TIME copyMargin = m_frameDuration > 0 ? m_frameDuration * 16 : DEFAULT_FRAME_DURATION * 16;
        const REFERENCE_TIME copyPixelsFromTime = position > copyMargin ? position - copyMargin : 0;
        {
            AutoCriticalSectionLock readerLock(&m_readerLock);
            clearVideoReorderFramesLocked();

            while (!m_videoSourceEnded && readCount < maxReadCount)
            {
                PlaybackVideoFrame current;
                bool sourceEnded = false;
                hr = readNextVideoFrameFromSourceLocked(current, sourceEnded, copyPixelsFromTime);
                if (FAILED(hr))
                {
                    break;
                }
                if (sourceEnded)
                {
                    m_videoSourceEnded = true;
                    endOfStream = true;
                    break;
                }

                ++readCount;
                if (current.m_pixels.empty())
                {
                    continue;
                }
                if (!m_videoReorderFrames.empty() && current.m_time < m_videoReorderFrames.back().m_time && m_videoReorderLogCount < 20)
                {
                    ++m_videoReorderLogCount;
                    Logf(L"Video reorder input detected during preview: previous=%s (%I64d), current=%s (%I64d), queue=%u",
                         FormatTime(m_videoReorderFrames.back().m_time).c_str(),
                         m_videoReorderFrames.back().m_time,
                         FormatTime(current.m_time).c_str(),
                         current.m_time,
                         static_cast<unsigned int>(m_videoReorderFrames.size()));
                }
                m_videoReorderFrames.push_back(current);
                if (current.m_time >= position && m_videoReorderFrames.size() >= kVideoReorderFrameCount)
                {
                    break;
                }
            }

            queueCount = m_videoReorderFrames.size();
            if (SUCCEEDED(hr) && !m_videoReorderFrames.empty())
            {
                std::stable_sort(m_videoReorderFrames.begin(), m_videoReorderFrames.end(), VideoFrameTimeLess);
                size_t bestIndex = 0;
                bool foundBeforeOrAtTarget = false;
                const REFERENCE_TIME targetTolerance = m_frameDuration / 3 > 1 ? m_frameDuration / 3 : 1;
                for (size_t i = 0; i < m_videoReorderFrames.size(); ++i)
                {
                    if (m_videoReorderFrames[i].m_time <= position + targetTolerance)
                    {
                        bestIndex = i;
                        foundBeforeOrAtTarget = true;
                    }
                    else if (!foundBeforeOrAtTarget)
                    {
                        bestIndex = i;
                        break;
                    }
                    else
                    {
                        break;
                    }
                }

                frame = m_videoReorderFrames[bestIndex];
                hasFrame = true;
            }
        }

        if (SUCCEEDED(hr) && hasFrame)
        {
            publishVideoFrame(frame);
        }

        Logf(L"Preview video frame after seek: position=%s (%I64d), hr=0x%08X, end=%d, hasFrame=%d, frameTime=%s (%I64d), read=%u, queue=%u, maxRead=%u, copyFrom=%s (%I64d)",
             FormatTime(position).c_str(),
             position,
             static_cast<unsigned int>(hr),
             endOfStream ? 1 : 0,
             hasFrame ? 1 : 0,
             FormatTime(frame.m_time).c_str(),
             frame.m_time,
             static_cast<unsigned int>(readCount),
             static_cast<unsigned int>(queueCount),
             static_cast<unsigned int>(maxReadCount),
             FormatTime(copyPixelsFromTime).c_str(),
             copyPixelsFromTime);
        return hr;
    }

    HRESULT MfSourcePlaybackEngine::copyVideoSample(IMFSample* sample, REFERENCE_TIME sampleTime, REFERENCE_TIME sampleDuration, PlaybackVideoFrame& frame)
    {
        if (!sample)
        {
            return E_POINTER;
        }
        if (m_videoWidth <= 0 || m_videoHeight <= 0)
        {
            return MF_E_INVALIDMEDIATYPE;
        }

        IMFMediaBuffer* buffer = nullptr;
        HRESULT hr = sample->ConvertToContiguousBuffer(&buffer);
        if (FAILED(hr))
        {
            return hr;
        }

        BYTE* data = nullptr;
        DWORD maxLength = 0;
        DWORD currentLength = 0;
        hr = buffer->Lock(&data, &maxLength, &currentLength);
        if (FAILED(hr))
        {
            SafeRelease(buffer);
            return hr;
        }

        const int destStride = m_videoWidth * 4;
        const int sourceStride = m_videoStride != 0 ? m_videoStride : destStride;
        const int absSourceStride = abs(sourceStride);
        const DWORD needed = static_cast<DWORD>(absSourceStride * m_videoHeight);
        if (currentLength < needed)
        {
            Logf(L"Video sample buffer shorter than expected: current=%lu, expected=%lu, stride=%d, height=%d",
                 currentLength,
                 needed,
                 sourceStride,
                 m_videoHeight);
        }

        frame.m_width = m_videoWidth;
        frame.m_height = m_videoHeight;
        frame.m_stride = destStride;
        frame.m_time = sampleTime;
        frame.m_duration = sampleDuration > 0 ? sampleDuration : m_frameDuration;
        frame.m_pixels.resize(destStride * m_videoHeight);

        // SourceReader输出的RGB32可能带正负stride，这里统一复制成GDI top-down DIB可直接绘制的行顺序。
        for (int y = 0; y < m_videoHeight; ++y)
        {
            BYTE* dest = &frame.m_pixels[0] + y * destStride;
            BYTE* source = nullptr;
            if (sourceStride >= 0)
            {
                source = data + y * absSourceStride;
            }
            else
            {
                source = data + (m_videoHeight - 1 - y) * absSourceStride;
            }
            CopyMemory(dest, source, min(destStride, absSourceStride));
        }

        buffer->Unlock();
        SafeRelease(buffer);
        return S_OK;
    }

    void MfSourcePlaybackEngine::publishVideoFrame(const PlaybackVideoFrame& frame)
    {
        {
            AutoCriticalSectionLock lock(&m_frameLock);
            if (m_lastPublishedVideoTime >= 0 && frame.m_time + m_frameDuration < m_lastPublishedVideoTime)
            {
                Logf(L"Video publish time moved backward: previous=%s (%I64d), current=%s (%I64d)",
                     FormatTime(m_lastPublishedVideoTime).c_str(),
                     m_lastPublishedVideoTime,
                     FormatTime(frame.m_time).c_str(),
                     frame.m_time);
            }

            m_currentFrame = frame;
            m_lastPublishedVideoTime = frame.m_time;
            ZeroMemory(&m_videoBitmapInfo, sizeof(m_videoBitmapInfo));
            m_videoBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            m_videoBitmapInfo.bmiHeader.biWidth = frame.m_width;
            m_videoBitmapInfo.bmiHeader.biHeight = -frame.m_height;
            m_videoBitmapInfo.bmiHeader.biPlanes = 1;
            m_videoBitmapInfo.bmiHeader.biBitCount = 32;
            m_videoBitmapInfo.bmiHeader.biCompression = BI_RGB;
        }

        if (m_videoWindow)
        {
            InvalidateRect(m_videoWindow, nullptr, FALSE);
        }
    }

    HRESULT MfSourcePlaybackEngine::readAndQueueAudio(bool& endOfStream)
    {
        endOfStream = false;
        if (!m_audioReader || !m_waveOut)
        {
            endOfStream = true;
            return S_OK;
        }

        int bufferIndex = findReusableAudioBuffer();
        if (bufferIndex < 0)
        {
            Sleep(kThreadWaitMs);
            return S_OK;
        }

        DWORD actualStream = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        IMFSample* sample = nullptr;
        HRESULT hr = S_OK;
        const DWORD sampleSeekSerial = readSeekSerial();
        {
            AutoCriticalSectionLock readerLock(&m_readerLock);
            hr = m_audioReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                           0,
                                           &actualStream,
                                           &flags,
                                           &timestamp,
                                           &sample);
        }

        if (FAILED(hr))
        {
            SafeRelease(sample);
            Logf(L"Audio ReadSample failed: hr=0x%08X", static_cast<unsigned int>(hr));
            return hr;
        }
        if (sampleSeekSerial != readSeekSerial())
        {
            SafeRelease(sample);
            Logf(L"Audio sample discarded after read because seek changed: oldSerial=%lu, currentSerial=%lu",
                 sampleSeekSerial,
                 readSeekSerial());
            return S_OK;
        }

        if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
        {
            Logf(L"Audio media type changed");
        }
        if ((flags & MF_SOURCE_READERF_ERROR) != 0)
        {
            SafeRelease(sample);
            Logf(L"Audio ReadSample returned error flag: flags=0x%08X", flags);
            return E_FAIL;
        }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
        {
            SafeRelease(sample);
            endOfStream = true;
            Logf(L"Audio end of stream reached");
            return S_OK;
        }
        if (!sample)
        {
            return S_OK;
        }

        IMFMediaBuffer* mediaBuffer = nullptr;
        hr = sample->ConvertToContiguousBuffer(&mediaBuffer);
        if (FAILED(hr))
        {
            SafeRelease(sample);
            return hr;
        }

        BYTE* data = nullptr;
        DWORD maxLength = 0;
        DWORD currentLength = 0;
        bool queuedAudioSample = false;
        hr = mediaBuffer->Lock(&data, &maxLength, &currentLength);
        bool bufferLocked = SUCCEEDED(hr);
        if (SUCCEEDED(hr) && currentLength > 0)
        {
            if (state() != PlaybackEngineStatePlaying)
            {
                mediaBuffer->Unlock();
                SafeRelease(mediaBuffer);
                SafeRelease(sample);
                return S_OK;
            }

            AutoCriticalSectionLock waveLock(&m_waveLock);
            if (sampleSeekSerial != readSeekSerial())
            {
                Logf(L"Audio sample discarded before waveOut queue because seek changed: time=%s (%I64d), oldSerial=%lu, currentSerial=%lu",
                     FormatTime(timestamp).c_str(),
                     timestamp,
                     sampleSeekSerial,
                     readSeekSerial());
            }
            else
            {
                PlaybackAudioBuffer& audioBuffer = m_audioBuffers[bufferIndex];
                unprepareAudioBuffer(static_cast<size_t>(bufferIndex));
                audioBuffer.m_data.assign(data, data + currentLength);
                ZeroMemory(&audioBuffer.m_header, sizeof(audioBuffer.m_header));
                audioBuffer.m_header.lpData = reinterpret_cast<LPSTR>(&audioBuffer.m_data[0]);
                audioBuffer.m_header.dwBufferLength = currentLength;

                MMRESULT mm = waveOutPrepareHeader(m_waveOut, &audioBuffer.m_header, sizeof(audioBuffer.m_header));
                if (mm == MMSYSERR_NOERROR)
                {
                    audioBuffer.m_prepared = true;
                    mm = waveOutWrite(m_waveOut, &audioBuffer.m_header, sizeof(audioBuffer.m_header));
                }
                if (mm != MMSYSERR_NOERROR)
                {
                    Logf(L"waveOut queue failed: mm=%u, bytes=%lu", static_cast<unsigned int>(mm), currentLength);
                    hr = HRESULT_FROM_WIN32(mm);
                }
                else
                {
                    m_nextAudioBuffer = (static_cast<size_t>(bufferIndex) + 1) % m_audioBuffers.size();
                    queuedAudioSample = true;
                }
            }
        }
        if (SUCCEEDED(hr) && queuedAudioSample)
        {
            ++m_audioSampleLogCount;
            if (m_audioSampleLogCount <= 5 || (m_audioSampleLogCount % 300) == 0)
            {
                LONGLONG sampleDuration = 0;
                sample->GetSampleDuration(&sampleDuration);
                Logf(L"Audio sample queued: index=%lu, time=%s (%I64d), duration=%s (%I64d), bytes=%lu, buffer=%d",
                     m_audioSampleLogCount,
                     FormatTime(timestamp).c_str(),
                     timestamp,
                     FormatTime(sampleDuration).c_str(),
                     sampleDuration,
                     currentLength,
                     bufferIndex);
            }
        }

        if (mediaBuffer && bufferLocked)
        {
            mediaBuffer->Unlock();
        }
        SafeRelease(mediaBuffer);
        SafeRelease(sample);
        return hr;
    }

    int MfSourcePlaybackEngine::findReusableAudioBuffer()
    {
        AutoCriticalSectionLock waveLock(&m_waveLock);
        if (m_audioBuffers.empty())
        {
            return -1;
        }

        for (size_t i = 0; i < m_audioBuffers.size(); ++i)
        {
            const size_t index = (m_nextAudioBuffer + i) % m_audioBuffers.size();
            PlaybackAudioBuffer& buffer = m_audioBuffers[index];
            if (!buffer.m_prepared || (buffer.m_header.dwFlags & WHDR_DONE) != 0)
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    void MfSourcePlaybackEngine::unprepareAudioBuffer(size_t index)
    {
        if (index >= m_audioBuffers.size() || !m_waveOut)
        {
            return;
        }

        PlaybackAudioBuffer& buffer = m_audioBuffers[index];
        if (buffer.m_prepared)
        {
            MMRESULT mm = waveOutUnprepareHeader(m_waveOut, &buffer.m_header, sizeof(buffer.m_header));
            if (mm != MMSYSERR_NOERROR)
            {
                Logf(L"waveOutUnprepareHeader failed: buffer=%u, mm=%u", static_cast<unsigned int>(index), static_cast<unsigned int>(mm));
            }
            buffer.m_prepared = false;
        }
        ZeroMemory(&buffer.m_header, sizeof(buffer.m_header));
        buffer.m_data.clear();
    }

    void MfSourcePlaybackEngine::resetWaveBuffers()
    {
        if (!m_waveOut)
        {
            return;
        }

        MMRESULT mm = waveOutReset(m_waveOut);
        Logf(L"waveOutReset returned %u", static_cast<unsigned int>(mm));
        for (size_t i = 0; i < m_audioBuffers.size(); ++i)
        {
            unprepareAudioBuffer(i);
        }
        m_nextAudioBuffer = 0;
        // seek和关闭路径会在持有wave锁时调用这里，避免反向再拿状态锁造成锁顺序风险。
        waveOutPause(m_waveOut);
    }

    void MfSourcePlaybackEngine::closeWaveOut()
    {
        AutoCriticalSectionLock lock(&m_waveLock);
        if (!m_waveOut)
        {
            return;
        }

        resetWaveBuffers();
        MMRESULT mm = waveOutClose(m_waveOut);
        Logf(L"waveOutClose returned %u", static_cast<unsigned int>(mm));
        m_waveOut = nullptr;
    }

    void MfSourcePlaybackEngine::resetClockLocked(REFERENCE_TIME position)
    {
        if (position < 0)
        {
            position = 0;
        }
        if (m_duration > 0 && position > m_duration)
        {
            position = m_duration;
        }

        m_clockBasePosition = position;
        QueryPerformanceCounter(&m_clockBaseCounter);
    }

    REFERENCE_TIME MfSourcePlaybackEngine::calculatePositionLocked() const
    {
        if (m_state != PlaybackEngineStatePlaying)
        {
            return m_clockBasePosition;
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        const LONGLONG elapsedCounter = now.QuadPart - m_clockBaseCounter.QuadPart;
        double elapsedSeconds = 0.0;
        if (m_counterFrequency.QuadPart > 0)
        {
            elapsedSeconds = static_cast<double>(elapsedCounter) / static_cast<double>(m_counterFrequency.QuadPart);
        }

        REFERENCE_TIME position = m_clockBasePosition + static_cast<REFERENCE_TIME>(elapsedSeconds * static_cast<double>(ONE_SECOND) * m_playbackRate + 0.5);
        if (position < 0)
        {
            position = 0;
        }
        if (m_duration > 0 && position > m_duration)
        {
            position = m_duration;
        }
        return position;
    }

    void MfSourcePlaybackEngine::setStateLocked(PlaybackEngineState stateValue, HRESULT result)
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        m_state = stateValue;
        m_lastResult = result;
        if (FAILED(result))
        {
            m_lastError = HResultText(result);
        }
    }

    void MfSourcePlaybackEngine::postEvent(PlaybackEngineEvent eventType, HRESULT result)
    {
        if (m_eventWindow && m_eventMessage != 0)
        {
            PostMessageW(m_eventWindow, m_eventMessage, static_cast<WPARAM>(eventType), static_cast<LPARAM>(result));
        }
    }

    void MfSourcePlaybackEngine::markStreamEnded(bool videoEnded, bool audioEnded)
    {
        bool shouldPostEnded = false;
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            if (videoEnded)
            {
                m_videoEnded = true;
            }
            if (audioEnded)
            {
                m_audioEnded = true;
            }

            const bool videoDone = !m_videoAvailable || m_videoEnded;
            const bool audioDone = !m_audioAvailable || m_audioEnded;
            if (m_state == PlaybackEngineStatePlaying && videoDone && audioDone)
            {
                resetClockLocked(m_duration);
                m_state = PlaybackEngineStateEnded;
                shouldPostEnded = true;
            }

            Logf(L"Engine stream ended update: videoEnded=%d, audioEnded=%d, storedVideo=%d, storedAudio=%d, videoAvailable=%d, audioAvailable=%d, state=%s, post=%d",
                 videoEnded ? 1 : 0,
                 audioEnded ? 1 : 0,
                 m_videoEnded ? 1 : 0,
                 m_audioEnded ? 1 : 0,
                 m_videoAvailable ? 1 : 0,
                 m_audioAvailable ? 1 : 0,
                 PlaybackEngineStateName(m_state),
                 shouldPostEnded ? 1 : 0);
        }

        if (shouldPostEnded)
        {
            postEvent(PlaybackEngineEventEnded, S_OK);
        }
    }

    DWORD MfSourcePlaybackEngine::readSeekSerial()
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        return m_seekSerial;
    }

    bool MfSourcePlaybackEngine::shouldExit() const
    {
        return m_isExit || (m_exitEvent && WaitForSingleObject(m_exitEvent, 0) == WAIT_OBJECT_0);
    }
}