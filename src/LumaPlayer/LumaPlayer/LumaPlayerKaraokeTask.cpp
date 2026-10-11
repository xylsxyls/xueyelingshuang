#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerKaraokeTask.h"
#include "Config.h"

#include "LumaPlayerCoreBridge.h"
#include "LumaPlayerMicrophoneCapture.h"
#include "LogManager/LogManagerAPI.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <limits>
#include <new>
#include <vector>

LumaPlayerKaraokeTask::LumaPlayerKaraokeTask(
    LumaPlayerMicrophoneCapture* capture, LumaPlayerCoreBridge* core, int32_t microphone) :
CTask(1),
m_capture(capture),
m_core(core),
m_microphone(microphone),
m_threadId(0),
m_exit(false),
m_done(true),
m_controlsVersion(0),
m_appliedVersion(0),
m_gainPercent(100),
m_muted(true),
m_monitoring(true),
m_recording(false),
m_timelineActive(true),
m_lastError(0),
m_lastInputPeak(0),
m_hasReceivedPcm(false),
m_maxProcessingMs(0),
m_recordingEnd100ns(-1),
m_recordingSnapshot100ns(-1),
m_recordingRate(0),
m_recordingSerial(0)
{

}

LumaPlayerKaraokeTask::~LumaPlayerKaraokeTask()
{
    uninit();
}

bool LumaPlayerKaraokeTask::init()
{
    if (m_threadId != 0)
    {
        return true;
    }
    try
    {
        m_exit.store(false);
        m_done.store(false);
        m_recordingEnd100ns = -1;
        m_recordingSnapshot100ns = -1;
        m_threadId = CTaskThreadManager::Instance().Init();
        std::shared_ptr<CTaskThread> thread =
            CTaskThreadManager::Instance().GetThreadInterface(m_threadId);
        if (thread == nullptr)
        {
            if (m_threadId != 0)
            {
                CTaskThreadManager::Instance().Uninit(m_threadId);
            }
            m_threadId = 0;
            m_done.store(true);
            return false;
        }
        /** 任务对象由麦克风槽位拥有，队列只借用而不销毁
        @param [in] task 借用的任务指针，槽位等待任务退出后才销毁
        */
        thread->PostTask(std::shared_ptr<CTask>(this, [](CTask* task)
                             {
                                 Q_UNUSED(task);
                             }),
            1);
    }
    catch (...)
    {
        if (m_threadId != 0)
        {
            CTaskThreadManager::Instance().Uninit(m_threadId);
        }
        m_threadId = 0;
        m_done.store(true);
        return false;
    }
    return true;
}

void LumaPlayerKaraokeTask::uninit()
{
    if (m_threadId == 0)
    {
        return;
    }
    StopTask();
    CTaskThreadManager::Instance().Uninit(m_threadId);
    m_threadId = 0;
}

void LumaPlayerKaraokeTask::setControls(int32_t gainPercent, bool muted,
    bool monitoring, bool recording, bool timelineActive)
{
    m_gainPercent.store((std::max)(0, (std::min)(5000, gainPercent)));
    m_muted.store(muted);
    m_monitoring.store(monitoring);
    m_recording.store(recording);
    m_timelineActive.store(timelineActive);
    m_controlsVersion.fetch_add(1);
    m_capture->setControls(gainPercent, muted, monitoring);
    m_wake.signal();
}

bool LumaPlayerKaraokeTask::recordingSettled() const
{
    return m_done.load() || (!m_recording.load() && m_appliedVersion.load() >= m_controlsVersion.load());
}

int32_t LumaPlayerKaraokeTask::lastError() const
{
    return m_lastError.load();
}

int32_t LumaPlayerKaraokeTask::lastInputPeak() const
{
    return m_capture->stats().m_peak;
}

bool LumaPlayerKaraokeTask::hasReceivedPcm() const
{
    return m_capture->stats().m_receivedFrames != 0;
}

uint64_t LumaPlayerKaraokeTask::monitorBytesWritten() const
{
    return m_capture->stats().m_monitoredFrames * 2;
}

int32_t LumaPlayerKaraokeTask::monitorPeriodMs() const
{
    return m_capture->stats().m_callbackFrames / 48;
}

int32_t LumaPlayerKaraokeTask::maxProcessingMs() const
{
    return m_maxProcessingMs.load();
}

int32_t LumaPlayerKaraokeTask::monitorBytesPerSecond() const
{
    return 48000 * 2;
}

void LumaPlayerKaraokeTask::DoTask()
{
    if (m_capture == nullptr || m_core == nullptr)
    {
        m_done.store(true);
        return;
    }
    try
    {
        if (!m_capture->startPrepared())
        {
            setError(-10, "microphone device could not start on audio worker");
            m_capture->stop();
            m_done.store(true);
            return;
        }
        uint64_t streamGeneration = 0;
        std::chrono::steady_clock::time_point lastDiagnostics = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point captureStart;
        bool waitingForPcm = false;
        bool noPcmReported = false;
        uint64_t previousReceived = 0;
        std::chrono::steady_clock::time_point lastPcm = std::chrono::steady_clock::now();
        while (!m_exit.load())
        {
            const uint64_t version = m_controlsVersion.load();
            if (m_capture->isRunning())
            {
                const uint64_t currentGeneration = m_capture->streamGeneration();
                if (currentGeneration != streamGeneration)
                {
                    streamGeneration = currentGeneration;
                    captureStart = std::chrono::steady_clock::now();
                    waitingForPcm = true;
                    noPcmReported = false;
                    m_hasReceivedPcm.store(false);
                    m_lastInputPeak.store(0);
                    m_lastError.store(0);
                    LOGINFO("Karaoke monitor capture stream changed generation=%llu",
                        static_cast<unsigned long long>(streamGeneration));
                }
            }
            else
            {
                waitingForPcm = false;
            }
            processAvailablePcm();
            m_appliedVersion.store(version);
            const LumaPlayerCoreCMicrophoneStats stats = m_capture->stats();
            if (stats.m_receivedFrames != previousReceived)
            {
                previousReceived = stats.m_receivedFrames;
                lastPcm = std::chrono::steady_clock::now();
            }
            if (stats.m_overflow || stats.m_error != 0 || !stats.m_running ||
                std::chrono::steady_clock::now() - lastPcm > std::chrono::seconds(2))
            {
                setError(stats.m_overflow ? -7 : -8, "Microphone queue overflow, interruption or missing PCM");
                m_recording.store(false);
                break;
            }
            if (m_hasReceivedPcm.load() && std::chrono::steady_clock::now() - lastDiagnostics > std::chrono::seconds(5))
            {
                LOGINFO("Native microphone callbackPeriodMs=%d maxProcessingMs=%d monitoredBytes=%llu",
                    monitorPeriodMs(), m_maxProcessingMs.load(),
                    static_cast<unsigned long long>(monitorBytesWritten()));
                lastDiagnostics = std::chrono::steady_clock::now();
            }
            if (waitingForPcm && !m_hasReceivedPcm.load() && !noPcmReported &&
                std::chrono::steady_clock::now() - captureStart > std::chrono::seconds(2))
            {
                int32_t expected = 0;
                if (m_lastError.compare_exchange_strong(expected, -8))
                {
                    LOGERROR("No microphone PCM received after capture start; check OS privacy permission and input level");
                }
                noPcmReported = true;
            }
            m_wake.wait((std::max)(1, g_config.m_microphoneServiceIntervalMs));
        }
    }
    catch (...)
    {
        m_lastError.store(-1);
        LOGERROR("Karaoke capture worker failed with an exception");
    }
    m_capture->stop();
    m_done.store(true);
}

void LumaPlayerKaraokeTask::setError(int32_t code, const char* message)
{
    const int32_t previous = m_lastError.exchange(code);
    if (code != 0 && previous != code)
    {
        LOGERROR("Karaoke microphone pipeline error=%d detail=%s", code,
            message != nullptr ? message : "unspecified");
    }
    else if (code == 0 && previous != 0)
    {
        LOGINFO("Karaoke microphone pipeline recovered");
    }
}

void LumaPlayerKaraokeTask::StopTask()
{
    m_exit.store(true);
    m_wake.signal();
}

void LumaPlayerKaraokeTask::processAvailablePcm()
{
    if (m_capture == nullptr || !m_capture->isRunning())
    {
        return;
    }
    const int32_t channels = m_capture->channels();
    const int32_t sampleRate = m_capture->sampleRate();
    const int32_t bytesPerFrame = channels * static_cast<int32_t>(sizeof(int16_t));
    if (sampleRate <= 0 || channels < 1 || channels > 2 || bytesPerFrame <= 0)
    {
        return;
    }
    QByteArray input;
    int32_t remainingMs = 0;
    while (!m_exit.load() && m_capture->takePcm(&input, sampleRate * bytesPerFrame / 100, &remainingMs))
    {
        const std::chrono::steady_clock::time_point processingStart = std::chrono::steady_clock::now();
        if (input.isEmpty() || input.size() % bytesPerFrame != 0)
        {
            setError(-5, "captured PCM is empty or contains an incomplete frame");
            return;
        }
        const size_t frameCount = static_cast<size_t>(input.size() / bytesPerFrame);
        int32_t peak = 0;
        const int16_t* inputSamples = reinterpret_cast<const int16_t*>(input.constData());
        for (size_t sample = 0; sample < frameCount * static_cast<size_t>(channels); ++sample)
        {
            const int32_t value = inputSamples[sample] < 0 ? -static_cast<int32_t>(inputSamples[sample]) : inputSamples[sample];
            peak = (std::max)(peak, value);
        }
        m_lastInputPeak.store(peak);
        if (!m_hasReceivedPcm.exchange(true))
        {
            LOGINFO("First microphone PCM block received rate=%d channels=%d frames=%llu peak=%d",
                sampleRate, channels, static_cast<unsigned long long>(frameCount), peak);
            setError(0, nullptr);
        }
        QByteArray monitor(input.size(), 0);
        const LumaPlayerCoreCSnapshot snapshot = m_timelineActive.load() ? m_core->snapshot() : LumaPlayerCoreCSnapshot();
        const bool playing = snapshot.m_state == LumaPlayerCoreCStatePlaying &&
            snapshot.m_position100ns >= 0 && snapshot.m_ratePermille >= 100 &&
            snapshot.m_ratePermille <= 5000;
        int64_t mediaStart = 0;
        int64_t mediaEnd = 0;
        if (playing)
        {
            const int64_t captureDuration = static_cast<int64_t>(frameCount) * 10000000LL / sampleRate;
            const int64_t mediaDuration = captureDuration * snapshot.m_ratePermille / 1000;
            const int64_t position = (std::max<int64_t>)(0, snapshot.m_recordingPosition100ns - static_cast<int64_t>(remainingMs) * snapshot.m_ratePermille * 10);
            // Small capture blocks can share a video snapshot. Advance by samples rather than overwrite them.
            const int64_t tolerance = (std::max<int64_t>)(1000000, snapshot.m_defaultVideoFrameDuration100ns * 2);
            const bool continuous = m_recordingEnd100ns >= 0 && m_recordingRate == snapshot.m_ratePermille &&
                m_recordingSerial == snapshot.m_recordingSerial &&
                snapshot.m_position100ns >= m_recordingSnapshot100ns &&
                position >= m_recordingEnd100ns - tolerance && position <= m_recordingEnd100ns + tolerance;
            mediaStart = continuous ? m_recordingEnd100ns : (std::max<int64_t>)(0, position - mediaDuration);
            mediaEnd = mediaStart + mediaDuration;
            m_recordingSnapshot100ns = snapshot.m_position100ns;
            m_recordingRate = snapshot.m_ratePermille;
            m_recordingSerial = snapshot.m_recordingSerial;
        }
        else
        {
            m_recordingEnd100ns = -1;
            m_recordingSnapshot100ns = -1;
        }
        const int32_t result = m_core->processKaraokeMicrophone(mediaStart,
            mediaEnd, playing ? snapshot.m_ratePermille : 1000,
            reinterpret_cast<const int16_t*>(input.constData()), frameCount,
            m_gainPercent.load(), m_muted.load(), m_monitoring.load(),
            m_recording.load(), playing && mediaEnd > mediaStart && m_timelineActive.load(),
            reinterpret_cast<int16_t*>(monitor.data()), m_microphone);
        if (result != LumaPlayerCoreCResultSuccess)
        {
            m_recordingEnd100ns = -1;
            setError(result, "Core rejected microphone PCM block");
            // A recording failure must not interrupt independent live monitoring.
            if (m_core->processKaraokeMicrophone(0, 0, 1000, inputSamples,
                    frameCount, m_gainPercent.load(), m_muted.load(), m_monitoring.load(),
                    false, false, reinterpret_cast<int16_t*>(monitor.data()), m_microphone) != LumaPlayerCoreCResultSuccess)
            {
                continue;
            }
        }
        else if (playing && m_recording.load() && m_timelineActive.load())
        {
            m_recordingEnd100ns = mediaEnd;
        }
        else
        {
            m_recordingEnd100ns = -1;
        }
        const int32_t processingMs = static_cast<int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - processingStart)
                .count());
        m_maxProcessingMs.store((std::max)(m_maxProcessingMs.load(), processingMs));
    }
    if (m_capture->hasOverflowed())
    {
        m_recording.store(false);
        setError(-7, "microphone PCM queue overflow");
    }
}