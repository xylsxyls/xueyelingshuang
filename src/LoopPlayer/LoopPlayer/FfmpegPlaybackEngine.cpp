#include "FfmpegPlaybackEngine.h"
#include "AutoCriticalSectionLock.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"
#include "FFmpegCpp/FFmpegCppAPI.h"

#include <algorithm>
#include <new>

namespace LoopPlayer
{
    // Audio decode has its own thread, so a slightly deeper queue is useful.
    static const DWORD kAudioBufferCount = 8;
    // Short poll interval used while workers are paused or waiting for buffers.
    static const DWORD kThreadWaitMs = 5;
    // Clock wait slice for responsive seek and exit behavior.
    static const DWORD kClockWaitSliceMs = 1;
    // Keep queued audio close to the master clock.
    static const REFERENCE_TIME kAudioMaxLead = ONE_SECOND / 4;
    // Decode enough frames after a keyframe seek to find the requested preview.
    static const size_t kDefaultPreviewFrameCount = 120;
    static const DWORD kThreadStopTimeoutMs = 5000;
    static const DWORD kVerboseFrameLogCount = 8;

    FfmpegPlaybackEngine::FfmpegPlaybackEngine()
        : m_videoReader(nullptr),
          m_audioReader(nullptr),
          m_previewReader(nullptr),
          m_videoWindow(nullptr),
          m_eventWindow(nullptr),
          m_eventMessage(0),
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
          m_state(PlaybackEngineStateClosed),
          m_lastResult(S_OK),
          m_seekSerial(0),
          m_duration(0),
          m_clockBasePosition(0),
          m_frameDuration(DEFAULT_FRAME_DURATION),
          m_playbackRate(1.0),
          m_videoWidth(0),
          m_videoHeight(0),
          m_videoFrameLogCount(0),
          m_audioFrameLogCount(0),
          m_dropFrameLogCount(0),
          m_lastPublishedVideoTime(-1)
    {
        InitializeCriticalSection(&m_stateLock);
        InitializeCriticalSection(&m_videoDecodeLock);
        InitializeCriticalSection(&m_audioDecodeLock);
        InitializeCriticalSection(&m_previewLock);
        InitializeCriticalSection(&m_frameLock);
        InitializeCriticalSection(&m_waveLock);
        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));
        ZeroMemory(&m_videoBitmapInfo, sizeof(m_videoBitmapInfo));
        QueryPerformanceFrequency(&m_counterFrequency);
        QueryPerformanceCounter(&m_clockBaseCounter);
    }

    FfmpegPlaybackEngine::~FfmpegPlaybackEngine()
    {
        uninit();
        DeleteCriticalSection(&m_waveLock);
        DeleteCriticalSection(&m_frameLock);
        DeleteCriticalSection(&m_previewLock);
        DeleteCriticalSection(&m_audioDecodeLock);
        DeleteCriticalSection(&m_videoDecodeLock);
        DeleteCriticalSection(&m_stateLock);
    }

    HRESULT FfmpegPlaybackEngine::init(const PlaybackEngineInitParam& param)
    {
        uninit();
        if (param.m_filePath.empty())
        {
            m_lastError = L"Playback file path is empty.";
            return E_INVALIDARG;
        }

        Logf(L"FfmpegPlaybackEngine init begin: path=%s, videoWindow=%p, eventWindow=%p, eventMessage=%u, fallbackFrame=%s (%I64d)",
             param.m_filePath.c_str(),
             param.m_videoWindow,
             param.m_eventWindow,
             param.m_eventMessage,
             FormatTime(param.m_fallbackFrameDuration).c_str(),
             param.m_fallbackFrameDuration);

        m_filePath = param.m_filePath;
        m_videoWindow = param.m_videoWindow;
        m_eventWindow = param.m_eventWindow;
        m_eventMessage = param.m_eventMessage;
        m_frameDuration = param.m_fallbackFrameDuration > 0 ? param.m_fallbackFrameDuration : DEFAULT_FRAME_DURATION;
        m_isExit.store(false);
        m_lastError.clear();
        m_videoEnded = false;
        m_audioEnded = false;
        m_videoFrameLogCount = 0;
        m_audioFrameLogCount = 0;
        m_dropFrameLogCount = 0;
        m_lastPublishedVideoTime = -1;

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

        HRESULT hr = openReaders(m_filePath);
        if (FAILED(hr))
        {
            uninit();
            return hr;
        }

        if (m_audioAvailable)
        {
            HRESULT audioHr = openWaveOut();
            if (FAILED(audioHr))
            {
                Logf(L"openWaveOut failed, continue video only: 0x%08X", static_cast<unsigned int>(audioHr));
                m_audioAvailable = false;
            }
        }

        if (!m_videoAvailable && !m_audioAvailable)
        {
            m_lastError = L"No playable video or audio stream found.";
            Logf(L"No playable streams after FFmpeg open");
            uninit();
            return E_FAIL;
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            m_isInit = true;
            m_state = PlaybackEngineStateStopped;
            m_lastResult = S_OK;
            resetClockLocked(0);
        }

        if (m_videoAvailable)
        {
            HRESULT previewHr = previewVideoFrame(0, kDefaultPreviewFrameCount);
            Logf(L"Initial preview returned 0x%08X", static_cast<unsigned int>(previewHr));
        }

        hr = startThreads();
        if (FAILED(hr))
        {
            uninit();
            return hr;
        }

        Logf(L"FfmpegPlaybackEngine init end: video=%d audio=%d native=%dx%d duration=%s (%I64d), frameDuration=%s (%I64d), audioFormat=%uHz/%uch/%ubit",
             m_videoAvailable ? 1 : 0,
             m_audioAvailable ? 1 : 0,
             m_videoWidth,
             m_videoHeight,
             FormatTime(m_duration).c_str(),
             m_duration,
             FormatTime(m_frameDuration).c_str(),
             m_frameDuration,
             static_cast<unsigned int>(m_waveFormat.nSamplesPerSec),
             static_cast<unsigned int>(m_waveFormat.nChannels),
             static_cast<unsigned int>(m_waveFormat.wBitsPerSample));
        return S_OK;
    }

    void FfmpegPlaybackEngine::uninit()
    {
        const bool needLog = m_isInit || m_videoReader || m_audioReader || m_previewReader || m_waveOut || m_videoThread || m_audioThread;
        if (needLog)
        {
            Logf(L"FfmpegPlaybackEngine uninit begin: videoThread=%p, audioThread=%p, waveOut=%p",
                 m_videoThread,
                 m_audioThread,
                 m_waveOut);
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            m_isExit.store(true);
            m_state = PlaybackEngineStateClosed;
            ++m_seekSerial;
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
            AutoCriticalSectionLock lock(&m_videoDecodeLock);
            if (m_videoReader)
            {
                m_videoReader->close();
                delete m_videoReader;
                m_videoReader = nullptr;
            }
        }

        {
            AutoCriticalSectionLock lock(&m_audioDecodeLock);
            if (m_audioReader)
            {
                m_audioReader->close();
                delete m_audioReader;
                m_audioReader = nullptr;
            }
        }

        {
            AutoCriticalSectionLock lock(&m_previewLock);
            if (m_previewReader)
            {
                m_previewReader->close();
                delete m_previewReader;
                m_previewReader = nullptr;
            }
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

        m_audioBuffers.clear();
        m_nextAudioBuffer = 0;
        m_filePath.clear();
        m_videoWindow = nullptr;
        m_eventWindow = nullptr;
        m_eventMessage = 0;
        m_isInit = false;
        m_isExit.store(false);
        m_videoAvailable = false;
        m_audioAvailable = false;
        m_videoEnded = false;
        m_audioEnded = false;
        m_state = PlaybackEngineStateClosed;
        m_lastResult = S_OK;
        m_duration = 0;
        m_clockBasePosition = 0;
        m_frameDuration = DEFAULT_FRAME_DURATION;
        m_playbackRate = 1.0;
        m_videoWidth = 0;
        m_videoHeight = 0;
        m_videoFrameLogCount = 0;
        m_audioFrameLogCount = 0;
        m_dropFrameLogCount = 0;
        m_lastPublishedVideoTime = -1;
        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));

        if (needLog)
        {
            Logf(L"FfmpegPlaybackEngine uninit end");
        }
    }

    HRESULT FfmpegPlaybackEngine::play()
    {
        if (!m_isInit)
        {
            return E_UNEXPECTED;
        }

        bool seekToStart = false;
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            const REFERENCE_TIME current = calculatePositionLocked();
            seekToStart = m_state == PlaybackEngineStateEnded || (m_duration > 0 && current >= m_duration);
        }

        if (seekToStart)
        {
            HRESULT seekHr = seek(0, false);
            if (FAILED(seekHr))
            {
                return seekHr;
            }
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            const REFERENCE_TIME current = calculatePositionLocked();
            resetClockLocked(current);
            m_videoEnded = false;
            m_audioEnded = false;
            m_state = PlaybackEngineStatePlaying;
            m_lastResult = S_OK;
            Logf(L"FFmpeg engine play: position=%s (%I64d), duration=%s (%I64d), rate=%.3f, serial=%lu",
                 FormatTime(m_clockBasePosition).c_str(),
                 m_clockBasePosition,
                 FormatTime(m_duration).c_str(),
                 m_duration,
                 m_playbackRate,
                 m_seekSerial);
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

    HRESULT FfmpegPlaybackEngine::pause()
    {
        if (!m_isInit)
        {
            return E_UNEXPECTED;
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            const REFERENCE_TIME current = calculatePositionLocked();
            resetClockLocked(current);
            m_state = PlaybackEngineStatePaused;
            m_lastResult = S_OK;
            Logf(L"FFmpeg engine pause: position=%s (%I64d), serial=%lu",
                 FormatTime(m_clockBasePosition).c_str(),
                 m_clockBasePosition,
                 m_seekSerial);
        }

        {
            AutoCriticalSectionLock waveLock(&m_waveLock);
            if (m_waveOut)
            {
                MMRESULT mm = waveOutPause(m_waveOut);
                Logf(L"waveOutPause returned %u", static_cast<unsigned int>(mm));
            }
        }
        if (m_wakeupEvent)
        {
            SetEvent(m_wakeupEvent);
        }
        return S_OK;
    }

    HRESULT FfmpegPlaybackEngine::stop()
    {
        if (!m_isInit)
        {
            return E_UNEXPECTED;
        }

        Logf(L"FFmpeg engine stop requested");
        HRESULT hr = seek(0, false);
        {
            AutoCriticalSectionLock lock(&m_stateLock);
            resetClockLocked(0);
            m_state = PlaybackEngineStateStopped;
            m_lastResult = hr;
        }
        return hr;
    }

    HRESULT FfmpegPlaybackEngine::seek(REFERENCE_TIME position, bool resumeIfPreviousPlaying, size_t previewMaxReadCount)
    {
        if (!m_isInit)
        {
            return E_UNEXPECTED;
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
            seekSerial = ++m_seekSerial;
        }

        resetWaveBuffers();

        const DWORD beginTick = GetTickCount();
        bool videoSeekOk = true;
        bool audioSeekOk = true;
        if (m_videoAvailable && m_videoReader)
        {
            AutoCriticalSectionLock lock(&m_videoDecodeLock);
            videoSeekOk = m_videoReader->seek(position);
        }
        if (m_audioAvailable && m_audioReader)
        {
            AutoCriticalSectionLock lock(&m_audioDecodeLock);
            audioSeekOk = m_audioReader->seek(position);
        }

        if (!videoSeekOk || !audioSeekOk)
        {
            if (!videoSeekOk)
            {
                setReaderError(m_videoReader, L"FFmpeg video seek failed");
            }
            else
            {
                setReaderError(m_audioReader, L"FFmpeg audio seek failed");
            }
            Logf(L"FFmpeg engine seek failed: target=%s (%I64d), serial=%lu, videoOk=%d, audioOk=%d, error=%s",
                 FormatTime(position).c_str(),
                 position,
                 seekSerial,
                 videoSeekOk ? 1 : 0,
                 audioSeekOk ? 1 : 0,
                 m_lastError.c_str());
            setStateLocked(PlaybackEngineStateError, E_FAIL);
            postEvent(PlaybackEngineEventError, E_FAIL);
            return E_FAIL;
        }

        HRESULT previewHr = S_FALSE;
        if (m_videoAvailable && m_previewReader)
        {
            previewHr = previewVideoFrame(position, previewMaxReadCount);
        }
        else
        {
            AutoCriticalSectionLock frameLock(&m_frameLock);
            m_lastPublishedVideoTime = -1;
        }

        {
            AutoCriticalSectionLock lock(&m_stateLock);
            resetClockLocked(position);
            m_state = previousState == PlaybackEngineStateStopped ? PlaybackEngineStateStopped : PlaybackEngineStatePaused;
            m_lastResult = S_OK;
        }

        Logf(L"FFmpeg engine seek end: target=%s (%I64d), previous=%s, resumePrevious=%d, previewHr=0x%08X, serial=%lu, elapsed=%lu",
             FormatTime(position).c_str(),
             position,
             PlaybackEngineStateName(previousState),
             resumeIfPreviousPlaying ? 1 : 0,
             static_cast<unsigned int>(previewHr),
             seekSerial,
             GetTickCount() - beginTick);

        postEvent(PlaybackEngineEventPositionSet, S_OK);
        if (m_wakeupEvent)
        {
            SetEvent(m_wakeupEvent);
        }
        if (previousState == PlaybackEngineStatePlaying && resumeIfPreviousPlaying)
        {
            return play();
        }
        return S_OK;
    }

    HRESULT FfmpegPlaybackEngine::previewVideoFrame(REFERENCE_TIME position, size_t previewMaxReadCount)
    {
        if (!m_isInit)
        {
            return E_UNEXPECTED;
        }
        if (!m_videoAvailable || m_previewReader == nullptr)
        {
            return S_FALSE;
        }
        if (position < 0)
        {
            position = 0;
        }
        if (m_duration > 0 && position > m_duration)
        {
            position = m_duration;
        }

        const size_t actualPreviewMax = previewMaxReadCount > 0 ? previewMaxReadCount : kDefaultPreviewFrameCount;
        const DWORD beginTick = GetTickCount();
        FFmpegCppPlaybackVideoFrame previewFrame;
        bool previewOk = false;
        {
            AutoCriticalSectionLock lock(&m_previewLock);
            previewOk = m_previewReader->readVideoFrameAt(position, actualPreviewMax, &previewFrame);
        }

        if (!previewOk)
        {
            setReaderError(m_previewReader, L"FFmpeg preview frame failed");
            Logf(L"FFmpeg previewVideoFrame failed: target=%s (%I64d), previewMax=%u, error=%s, elapsed=%lu",
                 FormatTime(position).c_str(),
                 position,
                 static_cast<unsigned int>(actualPreviewMax),
                 m_lastError.c_str(),
                 GetTickCount() - beginTick);
            return E_FAIL;
        }

        publishVideoFrame(previewFrame);
        Logf(L"FFmpeg previewVideoFrame succeeded: target=%s (%I64d), actualFrame=%s (%I64d), previewMax=%u, elapsed=%lu",
             FormatTime(position).c_str(),
             position,
             FormatTime(previewFrame.timestamp100ns).c_str(),
             previewFrame.timestamp100ns,
             static_cast<unsigned int>(actualPreviewMax),
             GetTickCount() - beginTick);
        return S_OK;
    }

    HRESULT FfmpegPlaybackEngine::setRate(double rate)
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

        Logf(L"FFmpeg engine setRate: rate=%.3f, waveOutSetPlaybackRate=%u", rate, static_cast<unsigned int>(mm));
        if (m_wakeupEvent)
        {
            SetEvent(m_wakeupEvent);
        }
        return S_OK;
    }

    bool FfmpegPlaybackEngine::getPosition(REFERENCE_TIME& position)
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

    void FfmpegPlaybackEngine::snapshot(PlaybackEngineSnapshot& snapshot)
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        snapshot.m_state = m_state;
        snapshot.m_lastResult = m_lastResult;
        snapshot.m_position = calculatePositionLocked();
        snapshot.m_duration = m_duration;
        snapshot.m_frameDuration = m_frameDuration;
        snapshot.m_playbackRate = m_playbackRate;
        snapshot.m_videoAvailable = m_videoAvailable;
        snapshot.m_audioAvailable = m_audioAvailable;
        snapshot.m_videoEnded = m_videoEnded;
        snapshot.m_audioEnded = m_audioEnded;
    }

    bool FfmpegPlaybackEngine::isOpen() const
    {
        return m_isInit;
    }

    REFERENCE_TIME FfmpegPlaybackEngine::duration() const
    {
        return m_duration;
    }

    REFERENCE_TIME FfmpegPlaybackEngine::frameDuration() const
    {
        return m_frameDuration;
    }

    bool FfmpegPlaybackEngine::getNativeVideoSize(int& width, int& height) const
    {
        width = m_videoWidth;
        height = m_videoHeight;
        return width > 0 && height > 0;
    }

    PlaybackEngineState FfmpegPlaybackEngine::state()
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        return m_state;
    }

    std::wstring FfmpegPlaybackEngine::lastError() const
    {
        return m_lastError;
    }

    void FfmpegPlaybackEngine::paintVideo(HDC hdc, const RECT& rect)
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

    HRESULT FfmpegPlaybackEngine::openReader(const std::wstring& filePath,
                                             bool decodeVideo,
                                             bool decodeAudio,
                                             FFmpegCppPlaybackReader** reader,
                                             const wchar_t* readerName)
    {
        if (!reader)
        {
            return E_POINTER;
        }

        *reader = new(std::nothrow) FFmpegCppPlaybackReader();
        if (!*reader)
        {
            m_lastError = L"Create FFmpeg reader failed.";
            return E_OUTOFMEMORY;
        }

        FFmpegCppPlaybackOpenOption option;
        option.decodeVideo = decodeVideo;
        option.decodeAudio = decodeAudio;
        option.outputChannels = 2;
        option.outputBitsPerSample = 16;
        option.fallbackVideoFrameDuration100ns = m_frameDuration;

        const std::string utf8Path = WideToUtf8String(filePath);
        const std::string ansiPath = WideToAnsiString(filePath);
        bool opened = (*reader)->open(utf8Path, option);
        Logf(L"FFmpeg %s reader open UTF-8 result=%d, decodeVideo=%d, decodeAudio=%d, error=%s",
             readerName ? readerName : L"",
             opened ? 1 : 0,
             decodeVideo ? 1 : 0,
             decodeAudio ? 1 : 0,
             Utf8ToWideString((*reader)->lastError()).c_str());

        if (!opened && ansiPath != utf8Path)
        {
            opened = (*reader)->open(ansiPath, option);
            Logf(L"FFmpeg %s reader open ANSI fallback result=%d, error=%s",
                 readerName ? readerName : L"",
                 opened ? 1 : 0,
                 Utf8ToWideString((*reader)->lastError()).c_str());
        }

        if (!opened)
        {
            setReaderError(*reader, L"FFmpeg open reader failed");
            (*reader)->close();
            delete *reader;
            *reader = nullptr;
            return E_FAIL;
        }
        return S_OK;
    }

    HRESULT FfmpegPlaybackEngine::openReaders(const std::wstring& filePath)
    {
        HRESULT videoHr = openReader(filePath, true, false, &m_videoReader, L"video");
        if (SUCCEEDED(videoHr) && m_videoReader && m_videoReader->hasVideo())
        {
            m_videoAvailable = true;
            m_duration = std::max<REFERENCE_TIME>(m_duration, m_videoReader->duration100ns());
            m_frameDuration = m_videoReader->videoFrameDuration100ns() > 0 ? m_videoReader->videoFrameDuration100ns() : m_frameDuration;
            int32_t videoWidth = 0;
            int32_t videoHeight = 0;
            if (m_videoReader->getVideoSize(&videoWidth, &videoHeight))
            {
                m_videoWidth = videoWidth;
                m_videoHeight = videoHeight;
            }
        }
        else
        {
            Logf(L"FFmpeg video reader unavailable: hr=0x%08X, error=%s",
                 static_cast<unsigned int>(videoHr),
                 m_lastError.c_str());
        }

        HRESULT audioHr = openReader(filePath, false, true, &m_audioReader, L"audio");
        if (SUCCEEDED(audioHr) && m_audioReader && m_audioReader->hasAudio())
        {
            m_audioAvailable = true;
            m_duration = std::max<REFERENCE_TIME>(m_duration, m_audioReader->duration100ns());
        }
        else
        {
            Logf(L"FFmpeg audio reader unavailable: hr=0x%08X, error=%s",
                 static_cast<unsigned int>(audioHr),
                 m_lastError.c_str());
        }

        if (!m_videoAvailable && !m_audioAvailable)
        {
            m_lastError = L"No playable FFmpeg stream.";
            return E_FAIL;
        }

        if (m_duration <= 0)
        {
            m_duration = m_frameDuration > 0 ? m_frameDuration : DEFAULT_FRAME_DURATION;
        }

        FFmpegCppPlaybackReader* infoReader = m_videoAvailable ? m_videoReader : m_audioReader;
        FFmpegCppMediaInfo mediaInfo;
        if (infoReader && infoReader->getMediaInfo(&mediaInfo))
        {
            FFmpegCppStreamInfo videoInfo;
            FFmpegCppStreamInfo audioInfo;
            const bool hasVideoInfo = mediaInfo.getMainVideoStreamInfo(&videoInfo);
            const bool hasAudioInfo = mediaInfo.getMainAudioStreamInfo(&audioInfo);
            Logf(L"FFmpeg media info: format=%s duration=%s (%I64d) bitRate=%I64d streams=%u video=%d codec=%s size=%dx%d fps=%d/%d audio=%d codec=%s sample=%d channels=%d",
                 Utf8ToWideString(mediaInfo.formatName).c_str(),
                 FormatTime(m_duration).c_str(),
                 m_duration,
                 mediaInfo.bitRate,
                 static_cast<unsigned int>(mediaInfo.streams.size()),
                 m_videoAvailable ? 1 : 0,
                 hasVideoInfo ? Utf8ToWideString(videoInfo.codecName).c_str() : L"",
                 m_videoWidth,
                 m_videoHeight,
                 hasVideoInfo ? videoInfo.frameRate.numerator : 0,
                 hasVideoInfo ? videoInfo.frameRate.denominator : 0,
                 m_audioAvailable ? 1 : 0,
                 hasAudioInfo ? Utf8ToWideString(audioInfo.codecName).c_str() : L"",
                 hasAudioInfo ? audioInfo.sampleRate : 0,
                 hasAudioInfo ? audioInfo.channels : 0);
        }

        if (m_videoAvailable)
        {
            HRESULT previewHr = openReader(filePath, true, false, &m_previewReader, L"preview");
            if (FAILED(previewHr))
            {
                Logf(L"FFmpeg preview reader unavailable: hr=0x%08X, playback can continue without preview",
                     static_cast<unsigned int>(previewHr));
            }
        }

        return S_OK;
    }

    HRESULT FfmpegPlaybackEngine::openWaveOut()
    {
        if (!m_audioReader || !m_audioReader->hasAudio())
        {
            return S_FALSE;
        }

        int32_t sampleRate = 0;
        int32_t channels = 0;
        int32_t bitsPerSample = 0;
        if (!m_audioReader->getAudioFormat(&sampleRate, &channels, &bitsPerSample) ||
            sampleRate <= 0 || channels <= 0 || bitsPerSample <= 0)
        {
            Logf(L"FFmpeg audio format unavailable");
            return E_FAIL;
        }

        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));
        m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        m_waveFormat.nChannels = static_cast<WORD>(channels);
        m_waveFormat.nSamplesPerSec = static_cast<DWORD>(sampleRate);
        m_waveFormat.wBitsPerSample = static_cast<WORD>(bitsPerSample);
        m_waveFormat.nBlockAlign = static_cast<WORD>(m_waveFormat.nChannels * m_waveFormat.wBitsPerSample / 8);
        m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;

        MMRESULT mm = waveOutOpen(&m_waveOut, WAVE_MAPPER, &m_waveFormat, 0, 0, CALLBACK_NULL);
        if (mm != MMSYSERR_NOERROR)
        {
            Logf(L"waveOutOpen failed: mm=%u, sample=%u, channels=%u, bits=%u",
                 static_cast<unsigned int>(mm),
                 static_cast<unsigned int>(m_waveFormat.nSamplesPerSec),
                 static_cast<unsigned int>(m_waveFormat.nChannels),
                 static_cast<unsigned int>(m_waveFormat.wBitsPerSample));
            return HRESULT_FROM_WIN32(mm);
        }

        m_audioBuffers.resize(kAudioBufferCount);
        m_nextAudioBuffer = 0;
        Logf(L"waveOutOpen succeeded: sample=%u, channels=%u, bits=%u, blockAlign=%u, avgBytes=%u, buffers=%u",
             static_cast<unsigned int>(m_waveFormat.nSamplesPerSec),
             static_cast<unsigned int>(m_waveFormat.nChannels),
             static_cast<unsigned int>(m_waveFormat.wBitsPerSample),
             static_cast<unsigned int>(m_waveFormat.nBlockAlign),
             static_cast<unsigned int>(m_waveFormat.nAvgBytesPerSec),
             static_cast<unsigned int>(m_audioBuffers.size()));
        return S_OK;
    }

    HRESULT FfmpegPlaybackEngine::startThreads()
    {
        DWORD threadId = 0;
        if (m_videoAvailable)
        {
            m_videoThread = CreateThread(nullptr, 0, FfmpegPlaybackEngine::VideoThreadProc, this, 0, &threadId);
            if (!m_videoThread)
            {
                const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                Logf(L"Create FFmpeg video thread failed: 0x%08X", static_cast<unsigned int>(hr));
                return hr;
            }
            Logf(L"FFmpeg video thread created: threadId=%lu", threadId);
        }

        if (m_audioAvailable)
        {
            threadId = 0;
            m_audioThread = CreateThread(nullptr, 0, FfmpegPlaybackEngine::AudioThreadProc, this, 0, &threadId);
            if (!m_audioThread)
            {
                const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                Logf(L"Create FFmpeg audio thread failed: 0x%08X", static_cast<unsigned int>(hr));
                return hr;
            }
            Logf(L"FFmpeg audio thread created: threadId=%lu", threadId);
        }
        return S_OK;
    }

    void FfmpegPlaybackEngine::stopThreads()
    {
        HANDLE threads[2] = { m_videoThread, m_audioThread };
        for (int i = 0; i < 2; ++i)
        {
            if (!threads[i])
            {
                continue;
            }
            const DWORD waitResult = WaitForSingleObject(threads[i], kThreadStopTimeoutMs);
            if (waitResult == WAIT_TIMEOUT)
            {
                Logf(L"FFmpeg worker thread stop timeout: handle=%p, waiting without timeout", threads[i]);
                WaitForSingleObject(threads[i], INFINITE);
            }
            CloseHandle(threads[i]);
        }
        m_videoThread = nullptr;
        m_audioThread = nullptr;
    }

    DWORD WINAPI FfmpegPlaybackEngine::VideoThreadProc(LPVOID param)
    {
        FfmpegPlaybackEngine* self = reinterpret_cast<FfmpegPlaybackEngine*>(param);
        if (self)
        {
            try
            {
                self->videoThreadMain();
            }
            catch (...)
            {
                self->m_lastError = L"Unknown exception in FFmpeg video thread.";
                self->setStateLocked(PlaybackEngineStateError, E_FAIL);
                self->postEvent(PlaybackEngineEventError, E_FAIL);
            }
        }
        return 0;
    }

    DWORD WINAPI FfmpegPlaybackEngine::AudioThreadProc(LPVOID param)
    {
        FfmpegPlaybackEngine* self = reinterpret_cast<FfmpegPlaybackEngine*>(param);
        if (self)
        {
            try
            {
                self->audioThreadMain();
            }
            catch (...)
            {
                self->m_lastError = L"Unknown exception in FFmpeg audio thread.";
                self->setStateLocked(PlaybackEngineStateError, E_FAIL);
                self->postEvent(PlaybackEngineEventError, E_FAIL);
            }
        }
        return 0;
    }

    void FfmpegPlaybackEngine::videoThreadMain()
    {
        Logf(L"FFmpeg video thread enter");
        while (!shouldExit())
        {
            if (!waitForPlayingState())
            {
                break;
            }

            const DWORD frameSeekSerial = readSeekSerial();
            FFmpegCppPlaybackFrame frame;
            int readResult = readNextVideoFrame(frame);
            if (readResult == FFmpegCppPlaybackReadResultError)
            {
                Logf(L"FFmpeg video read error: %s", m_lastError.c_str());
                setStateLocked(PlaybackEngineStateError, E_FAIL);
                postEvent(PlaybackEngineEventError, E_FAIL);
                break;
            }
            if (readResult == FFmpegCppPlaybackReadResultEnd)
            {
                Logf(L"FFmpeg video stream ended: serial=%lu", frameSeekSerial);
                markStreamEnded(true, false);
                while (!shouldExit() && frameSeekSerial == readSeekSerial() && state() == PlaybackEngineStatePlaying)
                {
                    Sleep(kThreadWaitMs);
                }
                continue;
            }
            if (frameSeekSerial != readSeekSerial() || frame.type != FFmpegCppPlaybackFrameTypeVideo)
            {
                continue;
            }

            const REFERENCE_TIME frameTime = frame.videoFrame.timestamp100ns;
            const REFERENCE_TIME frameDuration = frame.videoFrame.duration100ns > 0 ? frame.videoFrame.duration100ns : m_frameDuration;
            if (waitForVideoClock(frameTime, frameDuration, frameSeekSerial))
            {
                publishVideoFrame(frame.videoFrame);
                ++m_videoFrameLogCount;
                if (m_videoFrameLogCount <= kVerboseFrameLogCount || (m_videoFrameLogCount % 300) == 0)
                {
                    Logf(L"FFmpeg video frame publish: index=%lu, time=%s (%I64d), duration=%s (%I64d), key=%d, size=%dx%d, serial=%lu",
                         m_videoFrameLogCount,
                         FormatTime(frameTime).c_str(),
                         frameTime,
                         FormatTime(frameDuration).c_str(),
                         frameDuration,
                         frame.videoFrame.keyFrame ? 1 : 0,
                         frame.videoFrame.width,
                         frame.videoFrame.height,
                         frameSeekSerial);
                }
            }
        }
        Logf(L"FFmpeg video thread leave");
    }

    void FfmpegPlaybackEngine::audioThreadMain()
    {
        Logf(L"FFmpeg audio thread enter");
        while (!shouldExit())
        {
            if (!waitForPlayingState())
            {
                break;
            }

            const DWORD frameSeekSerial = readSeekSerial();
            FFmpegCppPlaybackFrame frame;
            int readResult = readNextAudioFrame(frame);
            if (readResult == FFmpegCppPlaybackReadResultError)
            {
                Logf(L"FFmpeg audio read error: %s", m_lastError.c_str());
                setStateLocked(PlaybackEngineStateError, E_FAIL);
                postEvent(PlaybackEngineEventError, E_FAIL);
                break;
            }
            if (readResult == FFmpegCppPlaybackReadResultEnd)
            {
                Logf(L"FFmpeg audio stream ended: serial=%lu", frameSeekSerial);
                markStreamEnded(false, true);
                while (!shouldExit() && frameSeekSerial == readSeekSerial() && state() == PlaybackEngineStatePlaying)
                {
                    Sleep(kThreadWaitMs);
                }
                continue;
            }
            if (frameSeekSerial != readSeekSerial() || frame.type != FFmpegCppPlaybackFrameTypeAudio)
            {
                continue;
            }

            const REFERENCE_TIME frameTime = frame.audioFrame.timestamp100ns;
            const REFERENCE_TIME frameDuration = frame.audioFrame.duration100ns;
            if (waitForAudioLead(frameTime, frameDuration, frameSeekSerial))
            {
                HRESULT hr = queueAudioFrame(frame.audioFrame, frameSeekSerial);
                if (FAILED(hr))
                {
                    setStateLocked(PlaybackEngineStateError, hr);
                    postEvent(PlaybackEngineEventError, hr);
                    break;
                }
            }
        }
        Logf(L"FFmpeg audio thread leave");
    }

    bool FfmpegPlaybackEngine::waitForPlayingState()
    {
        while (!shouldExit())
        {
            if (state() == PlaybackEngineStatePlaying)
            {
                return true;
            }
            if (m_wakeupEvent)
            {
                WaitForSingleObject(m_wakeupEvent, kThreadWaitMs);
            }
            else
            {
                Sleep(kThreadWaitMs);
            }
        }
        return false;
    }

    int FfmpegPlaybackEngine::readNextVideoFrame(FFmpegCppPlaybackFrame& frame)
    {
        AutoCriticalSectionLock lock(&m_videoDecodeLock);
        if (!m_videoReader)
        {
            m_lastError = L"FFmpeg video reader is null.";
            return FFmpegCppPlaybackReadResultError;
        }
        FFmpegCppPlaybackReadResult result = m_videoReader->read(&frame);
        if (result == FFmpegCppPlaybackReadResultError)
        {
            setReaderError(m_videoReader, L"FFmpeg video read failed");
        }
        return result;
    }

    int FfmpegPlaybackEngine::readNextAudioFrame(FFmpegCppPlaybackFrame& frame)
    {
        AutoCriticalSectionLock lock(&m_audioDecodeLock);
        if (!m_audioReader)
        {
            m_lastError = L"FFmpeg audio reader is null.";
            return FFmpegCppPlaybackReadResultError;
        }
        FFmpegCppPlaybackReadResult result = m_audioReader->read(&frame);
        if (result == FFmpegCppPlaybackReadResultError)
        {
            setReaderError(m_audioReader, L"FFmpeg audio read failed");
        }
        return result;
    }

    bool FfmpegPlaybackEngine::waitForVideoClock(REFERENCE_TIME frameTime, REFERENCE_TIME frameDuration, DWORD seekSerial)
    {
        for (;;)
        {
            if (shouldExit() || seekSerial != readSeekSerial())
            {
                return false;
            }
            if (state() != PlaybackEngineStatePlaying)
            {
                return false;
            }

            REFERENCE_TIME position = 0;
            getPosition(position);
            const REFERENCE_TIME lateTolerance = std::max<REFERENCE_TIME>(m_frameDuration * 2, ONE_SECOND / 10);
            if (frameTime + frameDuration + lateTolerance < position)
            {
                ++m_dropFrameLogCount;
                if (m_dropFrameLogCount <= kVerboseFrameLogCount || (m_dropFrameLogCount % 120) == 0)
                {
                    Logf(L"Late video frame dropped: frame=%s (%I64d), pos=%s (%I64d), duration=%s (%I64d), serial=%lu",
                         FormatTime(frameTime).c_str(),
                         frameTime,
                         FormatTime(position).c_str(),
                         position,
                         FormatTime(frameDuration).c_str(),
                         frameDuration,
                         seekSerial);
                }
                return false;
            }
            if (position + std::max<REFERENCE_TIME>(1, m_frameDuration / 3) >= frameTime)
            {
                return true;
            }
            Sleep(kClockWaitSliceMs);
        }
    }

    bool FfmpegPlaybackEngine::waitForAudioLead(REFERENCE_TIME frameTime, REFERENCE_TIME frameDuration, DWORD seekSerial)
    {
        for (;;)
        {
            if (shouldExit() || seekSerial != readSeekSerial())
            {
                return false;
            }
            if (state() != PlaybackEngineStatePlaying)
            {
                return false;
            }

            REFERENCE_TIME position = 0;
            getPosition(position);
            const REFERENCE_TIME lateTolerance = std::max<REFERENCE_TIME>(m_frameDuration * 2, ONE_SECOND / 10);
            if (frameTime + frameDuration + lateTolerance < position)
            {
                ++m_dropFrameLogCount;
                if (m_dropFrameLogCount <= kVerboseFrameLogCount || (m_dropFrameLogCount % 120) == 0)
                {
                    Logf(L"Late audio frame dropped: frame=%s (%I64d), pos=%s (%I64d), duration=%s (%I64d), serial=%lu",
                         FormatTime(frameTime).c_str(),
                         frameTime,
                         FormatTime(position).c_str(),
                         position,
                         FormatTime(frameDuration).c_str(),
                         frameDuration,
                         seekSerial);
                }
                return false;
            }
            if (frameTime <= position + kAudioMaxLead)
            {
                return true;
            }
            Sleep(kThreadWaitMs);
        }
    }

    void FfmpegPlaybackEngine::publishVideoFrame(const FFmpegCppPlaybackVideoFrame& frame)
    {
        if (frame.empty())
        {
            return;
        }

        {
            AutoCriticalSectionLock lock(&m_frameLock);
            if (m_lastPublishedVideoTime >= 0 && frame.timestamp100ns + m_frameDuration < m_lastPublishedVideoTime)
            {
                Logf(L"FFmpeg video publish time moved backward: previous=%s (%I64d), current=%s (%I64d)",
                     FormatTime(m_lastPublishedVideoTime).c_str(),
                     m_lastPublishedVideoTime,
                     FormatTime(frame.timestamp100ns).c_str(),
                     frame.timestamp100ns);
            }

            m_currentFrame.m_pixels = frame.bgraPixels;
            m_currentFrame.m_width = frame.width;
            m_currentFrame.m_height = frame.height;
            m_currentFrame.m_stride = frame.stride;
            m_currentFrame.m_time = frame.timestamp100ns;
            m_currentFrame.m_duration = frame.duration100ns > 0 ? frame.duration100ns : m_frameDuration;
            m_lastPublishedVideoTime = frame.timestamp100ns;

            ZeroMemory(&m_videoBitmapInfo, sizeof(m_videoBitmapInfo));
            m_videoBitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            m_videoBitmapInfo.bmiHeader.biWidth = frame.width;
            m_videoBitmapInfo.bmiHeader.biHeight = -frame.height;
            m_videoBitmapInfo.bmiHeader.biPlanes = 1;
            m_videoBitmapInfo.bmiHeader.biBitCount = 32;
            m_videoBitmapInfo.bmiHeader.biCompression = BI_RGB;
        }

        if (m_videoWindow)
        {
            InvalidateRect(m_videoWindow, nullptr, FALSE);
        }
    }

    HRESULT FfmpegPlaybackEngine::queueAudioFrame(const FFmpegCppPlaybackAudioFrame& frame, DWORD seekSerial)
    {
        if (frame.empty() || !m_waveOut)
        {
            return S_OK;
        }

        const int bufferIndex = findReusableAudioBuffer(seekSerial);
        if (bufferIndex < 0)
        {
            return S_OK;
        }

        if (seekSerial != readSeekSerial() || state() != PlaybackEngineStatePlaying)
        {
            return S_OK;
        }

        AutoCriticalSectionLock waveLock(&m_waveLock);
        if (!m_waveOut)
        {
            return S_OK;
        }

        PlaybackAudioBuffer& audioBuffer = m_audioBuffers[bufferIndex];
        unprepareAudioBuffer(static_cast<size_t>(bufferIndex));
        audioBuffer.m_data = frame.pcmData;
        ZeroMemory(&audioBuffer.m_header, sizeof(audioBuffer.m_header));
        audioBuffer.m_header.lpData = reinterpret_cast<LPSTR>(&audioBuffer.m_data[0]);
        audioBuffer.m_header.dwBufferLength = static_cast<DWORD>(audioBuffer.m_data.size());

        MMRESULT mm = waveOutPrepareHeader(m_waveOut, &audioBuffer.m_header, sizeof(audioBuffer.m_header));
        if (mm == MMSYSERR_NOERROR)
        {
            audioBuffer.m_prepared = true;
            mm = waveOutWrite(m_waveOut, &audioBuffer.m_header, sizeof(audioBuffer.m_header));
        }

        if (mm != MMSYSERR_NOERROR)
        {
            Logf(L"waveOut queue failed: mm=%u, bytes=%u, time=%s (%I64d), duration=%s (%I64d), serial=%lu",
                 static_cast<unsigned int>(mm),
                 static_cast<unsigned int>(audioBuffer.m_data.size()),
                 FormatTime(frame.timestamp100ns).c_str(),
                 frame.timestamp100ns,
                 FormatTime(frame.duration100ns).c_str(),
                 frame.duration100ns,
                 seekSerial);
            return HRESULT_FROM_WIN32(mm);
        }

        m_nextAudioBuffer = (static_cast<size_t>(bufferIndex) + 1) % m_audioBuffers.size();
        ++m_audioFrameLogCount;
        if (m_audioFrameLogCount <= kVerboseFrameLogCount || (m_audioFrameLogCount % 300) == 0)
        {
            Logf(L"FFmpeg audio frame queued: index=%lu, time=%s (%I64d), duration=%s (%I64d), bytes=%u, buffer=%d, serial=%lu",
                 m_audioFrameLogCount,
                 FormatTime(frame.timestamp100ns).c_str(),
                 frame.timestamp100ns,
                 FormatTime(frame.duration100ns).c_str(),
                 frame.duration100ns,
                 static_cast<unsigned int>(audioBuffer.m_data.size()),
                 bufferIndex,
                 seekSerial);
        }
        return S_OK;
    }

    int FfmpegPlaybackEngine::findReusableAudioBuffer(DWORD seekSerial)
    {
        for (;;)
        {
            if (shouldExit() || seekSerial != readSeekSerial() || state() != PlaybackEngineStatePlaying)
            {
                return -1;
            }
            {
                AutoCriticalSectionLock waveLock(&m_waveLock);
                if (!m_waveOut || m_audioBuffers.empty())
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
            }
            Sleep(kThreadWaitMs);
        }
    }

    void FfmpegPlaybackEngine::unprepareAudioBuffer(size_t index)
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
                Logf(L"waveOutUnprepareHeader failed: buffer=%u, mm=%u",
                     static_cast<unsigned int>(index),
                     static_cast<unsigned int>(mm));
            }
            buffer.m_prepared = false;
        }
        buffer.m_data.clear();
        ZeroMemory(&buffer.m_header, sizeof(buffer.m_header));
    }

    void FfmpegPlaybackEngine::resetWaveBuffers()
    {
        AutoCriticalSectionLock waveLock(&m_waveLock);
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
        waveOutPause(m_waveOut);
    }

    void FfmpegPlaybackEngine::closeWaveOut()
    {
        AutoCriticalSectionLock waveLock(&m_waveLock);
        if (!m_waveOut)
        {
            return;
        }

        waveOutReset(m_waveOut);
        for (size_t i = 0; i < m_audioBuffers.size(); ++i)
        {
            unprepareAudioBuffer(i);
        }
        MMRESULT mm = waveOutClose(m_waveOut);
        Logf(L"waveOutClose returned %u", static_cast<unsigned int>(mm));
        m_waveOut = nullptr;
    }

    void FfmpegPlaybackEngine::resetClockLocked(REFERENCE_TIME position)
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

    REFERENCE_TIME FfmpegPlaybackEngine::calculatePositionLocked() const
    {
        if (m_state != PlaybackEngineStatePlaying)
        {
            return m_clockBasePosition;
        }

        LARGE_INTEGER now = { 0 };
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

    void FfmpegPlaybackEngine::setStateLocked(PlaybackEngineState stateValue, HRESULT result)
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        m_state = stateValue;
        m_lastResult = result;
        if (FAILED(result))
        {
            m_lastError = HResultText(result);
        }
    }

    void FfmpegPlaybackEngine::postEvent(PlaybackEngineEvent eventType, HRESULT result)
    {
        if (m_eventWindow && m_eventMessage != 0)
        {
            PostMessageW(m_eventWindow, m_eventMessage, static_cast<WPARAM>(eventType), static_cast<LPARAM>(result));
        }
    }

    void FfmpegPlaybackEngine::markStreamEnded(bool videoEnded, bool audioEnded)
    {
        bool shouldPostEnded = false;
        bool currentVideoEnded = false;
        bool currentAudioEnded = false;
        bool allVideoDone = false;
        bool allAudioDone = false;
        PlaybackEngineState stateValue = PlaybackEngineStateClosed;
        REFERENCE_TIME duration = 0;
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

            allVideoDone = !m_videoAvailable || m_videoEnded;
            allAudioDone = !m_audioAvailable || m_audioEnded;
            if (allVideoDone && allAudioDone && m_state == PlaybackEngineStatePlaying)
            {
                resetClockLocked(m_duration);
                m_state = PlaybackEngineStateEnded;
                m_lastResult = S_OK;
                shouldPostEnded = true;
            }

            currentVideoEnded = m_videoEnded;
            currentAudioEnded = m_audioEnded;
            stateValue = m_state;
            duration = m_duration;
        }

        Logf(L"FFmpeg stream ended update: videoEnded=%d audioEnded=%d allVideoDone=%d allAudioDone=%d state=%s post=%d duration=%s (%I64d)",
             currentVideoEnded ? 1 : 0,
             currentAudioEnded ? 1 : 0,
             allVideoDone ? 1 : 0,
             allAudioDone ? 1 : 0,
             PlaybackEngineStateName(stateValue),
             shouldPostEnded ? 1 : 0,
             FormatTime(duration).c_str(),
             duration);

        if (shouldPostEnded)
        {
            postEvent(PlaybackEngineEventEnded, S_OK);
        }
    }

    DWORD FfmpegPlaybackEngine::readSeekSerial()
    {
        AutoCriticalSectionLock lock(&m_stateLock);
        return m_seekSerial;
    }

    bool FfmpegPlaybackEngine::shouldExit() const
    {
        return m_isExit.load() || (m_exitEvent && WaitForSingleObject(m_exitEvent, 0) == WAIT_OBJECT_0);
    }

    void FfmpegPlaybackEngine::setReaderError(const FFmpegCppPlaybackReader* reader, const wchar_t* prefix)
    {
        std::wstring text = prefix ? prefix : L"FFmpeg error";
        if (reader)
        {
            const std::wstring readerError = Utf8ToWideString(reader->lastError());
            if (!readerError.empty())
            {
                text += L": ";
                text += readerError;
            }
        }
        m_lastError = text;
    }
}