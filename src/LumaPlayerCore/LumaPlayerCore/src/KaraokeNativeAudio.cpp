#include "KaraokeNativeAudio.h"
#include "KaraokeNativeBackend.h"
#include <algorithm>
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#include <audioclient.h>
#endif
#include "LogManager/LogManagerAPI.h"

/** 仅记录设备初始化诊断，不在实时音频回调注册
@param [in] userData miniaudio传入上下文，本回调不使用
@param [in] level miniaudio日志级别，本适配器统一转交INFO
@param [in] message 借用的诊断文本，空值使用入口约定的默认文本
*/
static void NativeInitializationLog(void* userData, ma_uint32 level, const char* message)
{
    LOGINFO("Microphone device initialization: %s", message);
}

/** 转发双工PCM回调，不分配或加锁
@param [in] device 有效miniaudio设备，pUserData借用KaraokeNativeAudio
@param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
@param [in] input 借用的PCM输入，长度由采样数或帧数乘声道数决定
@param [in] frames 单声道PCM帧数
*/
static void NativeMicrophoneCallback(ma_device* device, void* output, const void* input, ma_uint32 frames)
{
    static_cast<KaraokeNativeAudio*>(device->pUserData)->process(static_cast<const int16_t*>(input), static_cast<int16_t*>(output), frames);
}

/** 转发设备停止事件到原子状态，不释放设备
@param [in] notification 借用的后端通知，仅回调期间有效
*/
static void NativeMicrophoneNotification(const ma_device_notification* notification)
{
    if (notification->type == ma_device_notification_type_stopped ||
        notification->type == ma_device_notification_type_interruption_began)
    {
        static_cast<KaraokeNativeAudio*>(notification->pDevice->pUserData)->interrupted();
    }
}

KaraokeNativeBackend::KaraokeNativeBackend() :
m_context(),
m_device(),
m_inputId(),
m_contextReady(false),
m_deviceReady(false),
m_periodMs(10)
{

}

KaraokeNativeAudio::KaraokeNativeAudio() :
m_backend(new KaraokeNativeBackend()),
m_written(0),
m_read(0),
m_received(0),
m_monitored(0),
m_controls(KaraokeMicrophoneControls::encode(100, true, true)),
m_running(false),
m_overflow(false),
m_peak(0),
m_error(0),
m_periodFrames(0),
m_captureAgeMs(-1),
m_maxCaptureAgeMs(-1),
m_outputPaddingMs(-1),
m_counterFrequency(0),
m_nextDiagnosticFrame(0)
{
#ifdef _WIN32
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency))
    {
        m_counterFrequency = frequency.QuadPart;
    }
#endif
}

KaraokeNativeAudio::~KaraokeNativeAudio()
{
    stop();
}

int32_t KaraokeNativeAudio::enumerate(LumaPlayerCoreCMicrophoneNameCallback callback, void* userData)
{
    if (callback == nullptr)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS)
    {
        return LumaPlayerCoreCResultOpenFailed;
    }
    ma_device_info* inputs = nullptr;
    ma_uint32 count = 0;
    const ma_result result = ma_context_get_devices(&context, nullptr, nullptr, &inputs, &count);
    if (result == MA_SUCCESS)
    {
        for (ma_uint32 i = 0; i < count; ++i)
        {
            try
            {
                callback(static_cast<int32_t>(i), inputs[i].name, userData);
            }
            catch (...)
            {
                ma_context_uninit(&context);
                return LumaPlayerCoreCResultInternalError;
            }
        }
    }
    ma_context_uninit(&context);
    return result == MA_SUCCESS ? 0 : LumaPlayerCoreCResultOpenFailed;
}

int32_t KaraokeNativeAudio::prepare(int32_t index, int32_t queueMs, int32_t periodMs)
{
    stop();
    m_error.store(0);
    if (index < 0 || queueMs < 200 || queueMs > 10000 || periodMs < 5 || periodMs > 200)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    if (ma_context_init(nullptr, 0, nullptr, &m_backend->m_context) != MA_SUCCESS)
    {
        return LumaPlayerCoreCResultOpenFailed;
    }
    m_backend->m_contextReady = true;
    ma_device_info* inputs = nullptr;
    ma_uint32 count = 0;
    if (ma_context_get_devices(&m_backend->m_context, nullptr, nullptr, &inputs, &count) != MA_SUCCESS ||
        static_cast<uint32_t>(index) >= count)
    {
        stop();
        return LumaPlayerCoreCResultOpenFailed;
    }
    m_backend->m_inputId = inputs[index].id;
    m_backend->m_periodMs = periodMs;
    try
    {
        m_ring.assign(static_cast<size_t>(48 * queueMs), 0);
    }
    catch (...)
    {
        stop();
        m_error.store(LumaPlayerCoreCResultInternalError);
        return LumaPlayerCoreCResultInternalError;
    }
    m_written.store(0);
    m_read.store(0);
    m_received.store(0);
    m_monitored.store(0);
    m_overflow.store(false);
    m_peak.store(0);
    m_periodFrames.store(0);
    m_captureAgeMs.store(-1);
    m_maxCaptureAgeMs.store(-1);
    m_outputPaddingMs.store(-1);
    m_nextDiagnosticFrame = 48000;
    return 0;
}

int32_t KaraokeNativeAudio::start()
{
    if (!m_backend->m_contextReady || m_backend->m_deviceReady || m_ring.empty())
    {
        return LumaPlayerCoreCResultStateError;
    }
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.pDeviceID = &m_backend->m_inputId;
    config.capture.format = ma_format_s16;
    config.capture.channels = 1;
    config.playback.format = ma_format_s16;
    config.playback.channels = 1;
    config.sampleRate = 48000;
    config.periodSizeInMilliseconds = m_backend->m_periodMs;
    config.periods = 2;
    config.noFixedSizedCallback = MA_TRUE;
    config.wasapi.noAutoConvertSRC = MA_TRUE;
    config.wasapi.usage = ma_wasapi_usage_pro_audio;
    config.wasapi.noHardwareOffloading = MA_TRUE;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.dataCallback = NativeMicrophoneCallback;
    config.notificationCallback = NativeMicrophoneNotification;
    config.pUserData = this;
    const ma_log_callback diagnostic = ma_log_callback_init(NativeInitializationLog, nullptr);
    ma_log_register_callback(ma_context_get_log(&m_backend->m_context), diagnostic);
    ma_result result = ma_device_init(&m_backend->m_context, &config, &m_backend->m_device);
    ma_log_unregister_callback(ma_context_get_log(&m_backend->m_context), diagnostic);
    if (result == MA_SUCCESS)
    {
        m_backend->m_deviceReady = true;
        const ma_device& device = m_backend->m_device;
        LOGINFO("Microphone backend=%s input=%s output=%s requestedPeriodMs=%d captureRate=%u playbackRate=%u capturePeriod=%u playbackPeriod=%u",
            ma_get_backend_name(m_backend->m_context.backend), device.capture.name, device.playback.name,
            m_backend->m_periodMs, device.capture.internalSampleRate, device.playback.internalSampleRate,
            device.capture.internalPeriodSizeInFrames, device.playback.internalPeriodSizeInFrames);
#ifdef _WIN32
        if (m_backend->m_context.backend == ma_backend_wasapi)
        {
            LOGINFO("Microphone WASAPI captureBuffer=%u playbackBuffer=%u; buffer frames are not acoustic round-trip latency",
                device.wasapi.actualBufferSizeInFramesCapture, device.wasapi.actualBufferSizeInFramesPlayback);
            REFERENCE_TIME captureLatency = -1, playbackLatency = -1;
            IAudioClient* capture = static_cast<IAudioClient*>(device.wasapi.pAudioClientCapture);
            IAudioClient* playback = static_cast<IAudioClient*>(device.wasapi.pAudioClientPlayback);
            if (capture)
            {
                capture->GetStreamLatency(&captureLatency);
            }
            if (playback)
            {
                playback->GetStreamLatency(&playbackLatency);
            }
            LOGINFO("Microphone driver-reported maximum stream latency capture100ns=%lld playback100ns=%lld; not measured acoustic latency",
                static_cast<long long>(captureLatency), static_cast<long long>(playbackLatency));
        }
#endif
        m_running.store(true);
        result = ma_device_start(&m_backend->m_device);
    }
    if (result != MA_SUCCESS)
    {
        LOGERROR("Microphone device initialization/start failed: nativeResult=%d", static_cast<int32_t>(result));
        m_error.store(static_cast<int32_t>(result));
        stop();
        return LumaPlayerCoreCResultOpenFailed;
    }
    return m_running.load() ? 0 : LumaPlayerCoreCResultOpenFailed;
}

void KaraokeNativeAudio::interrupted()
{
    if (m_running.exchange(false))
    {
        m_error.store(-300);
    }
}

void KaraokeNativeAudio::observeCapturePacket(uint64_t timestamp, uint32_t flags, uint32_t padding, uint32_t rate)
{
#ifdef _WIN32
    // WASAPI timestamps are already in 100 ns units, unlike the raw QPC counter.
    LARGE_INTEGER counter;
    if (timestamp && !(flags & 4) && m_counterFrequency > 0 && QueryPerformanceCounter(&counter))
    {
        const uint64_t now = static_cast<uint64_t>(counter.QuadPart / m_counterFrequency) * 10000000 +
            static_cast<uint64_t>(counter.QuadPart % m_counterFrequency) * 10000000 / m_counterFrequency;
        if (now >= timestamp && now - timestamp < 100000000)
        {
            const int32_t age = static_cast<int32_t>((now - timestamp) / 10000);
            m_captureAgeMs.store(age);
            m_maxCaptureAgeMs.store((std::max)(m_maxCaptureAgeMs.load(), age));
        }
    }
#endif
    if (rate)
    {
        m_outputPaddingMs.store(static_cast<int32_t>(static_cast<uint64_t>(padding) * 1000 / rate));
    }
}

void KaraokeNativeAudio::stop()
{
    m_running.store(false);
    if (m_backend->m_deviceReady)
    {
        ma_device_uninit(&m_backend->m_device);
        m_backend->m_deviceReady = false;
    }
    if (m_backend->m_contextReady)
    {
        ma_context_uninit(&m_backend->m_context);
        m_backend->m_contextReady = false;
    }
}

int32_t KaraokeNativeAudio::controls(int32_t gain, bool muted, bool monitoring)
{
    if (gain < 0 || gain > 5000)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    m_controls.store(KaraokeMicrophoneControls::encode(gain, muted, monitoring));
    return 0;
}

void KaraokeNativeAudio::process(const int16_t* input, int16_t* output, uint32_t frames)
{
    const uint32_t control = m_controls.load();
    const int32_t gain = KaraokeMicrophoneControls::gain(control);
    const bool audible = !KaraokeMicrophoneControls::muted(control) && KaraokeMicrophoneControls::monitoring(control);
    int32_t peak = 0;
    for (uint32_t i = 0; i < frames; ++i)
    {
        const int32_t value = input != nullptr ? input[i] : 0;
        peak = (std::max)(peak, value < 0 ? -value : value);
        if (output != nullptr)
        {
            output[i] = audible ? static_cast<int16_t>((std::max)(-32768,
                                      (std::min)(32767, value * gain / 100)))
                                : 0;
        }
    }
    m_peak.store(peak);
    m_periodFrames.store(static_cast<int32_t>(frames));
    if (input == nullptr)
    {
        return;
    }
    m_received.fetch_add(frames);
    if (audible && output != nullptr)
    {
        m_monitored.fetch_add(frames);
    }
    const uint64_t write = m_written.load(std::memory_order_relaxed);
    const uint64_t read = m_read.load(std::memory_order_acquire);
    if (m_overflow.load() || frames > m_ring.size() - static_cast<size_t>(write - read))
    {
        m_overflow.store(true);
        return;
    }
    const size_t offset = static_cast<size_t>(write % m_ring.size());
    const size_t first = (std::min<size_t>)(frames, m_ring.size() - offset);
    std::memcpy(m_ring.data() + offset, input, first * sizeof(int16_t));
    std::memcpy(m_ring.data(), input + first, (frames - first) * sizeof(int16_t));
    m_written.store(write + frames, std::memory_order_release);
}

size_t KaraokeNativeAudio::read(int16_t* output, size_t capacity, int32_t* remainingMs)
{
    if (output == nullptr || capacity == 0 || m_ring.empty())
    {
        return 0;
    }
    const uint64_t read = m_read.load(std::memory_order_relaxed);
    const uint64_t write = m_written.load(std::memory_order_acquire);
    if (write >= m_nextDiagnosticFrame)
    {
        m_nextDiagnosticFrame = write + 240000;
        LOGINFO("Microphone timing input=%s capturePacketAgeMs=%d maxCapturePacketAgeMs=%d outputPaddingMs=%d recordQueueMs=%llu gain=%d; excludes hardware/acoustic latency",
            m_backend->m_device.capture.name, m_captureAgeMs.load(), m_maxCaptureAgeMs.load(), m_outputPaddingMs.load(),
            static_cast<unsigned long long>((write - read) / 48), KaraokeMicrophoneControls::gain(m_controls.load()));
    }
    const size_t count = (std::min)(capacity, static_cast<size_t>(write - read));
    const size_t offset = static_cast<size_t>(read % m_ring.size());
    const size_t first = (std::min)(count, m_ring.size() - offset);
    std::memcpy(output, m_ring.data() + offset, first * sizeof(int16_t));
    std::memcpy(output + first, m_ring.data(), (count - first) * sizeof(int16_t));
    m_read.store(read + count, std::memory_order_release);
    if (remainingMs != nullptr)
    {
        *remainingMs = static_cast<int32_t>((write - read - count) / 48);
    }
    return count;
}

void KaraokeNativeAudio::stats(LumaPlayerCoreCMicrophoneStats* value)
{
    if (value != nullptr)
    {
        value->m_running = m_running.load() ? 1 : 0;
        value->m_overflow = m_overflow.load() ? 1 : 0;
        value->m_receivedFrames = m_received.load();
        value->m_monitoredFrames = m_monitored.load();
        value->m_peak = m_peak.load();
        value->m_error = m_error.load();
        value->m_callbackFrames = m_periodFrames.load();
    }
}