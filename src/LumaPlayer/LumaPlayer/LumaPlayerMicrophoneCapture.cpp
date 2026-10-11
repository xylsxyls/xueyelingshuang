#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerMicrophoneCapture.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>

LumaPlayerMicrophoneCapture::LumaPlayerMicrophoneCapture() :
m_handle(lumaPlayerCoreMicrophoneCreate()),
m_prepared(false),
m_error(0),
m_generation(0)
{

}

LumaPlayerMicrophoneCapture::~LumaPlayerMicrophoneCapture()
{
    lumaPlayerCoreMicrophoneDestroy(m_handle);
}

QStringList LumaPlayerMicrophoneCapture::availableDeviceNames()
{
    QStringList result;
    /** 将原生设备名称复制到界面名称列表
    @param [in] index 枚举下标，此列表按回调顺序保存而不单独使用下标
    @param [in] name 借用的UTF-8设备名称，回调返回前复制
    @param [in,out] userData 指向本次枚举的QStringList，不转移所有权
    */
    lumaPlayerCoreMicrophoneEnumerate([](int32_t index, const char* name, void* userData)
        {
            Q_UNUSED(index);
            static_cast<QStringList*>(userData)->append(QString::fromUtf8(name));
        },
        &result);
    return result;
}

bool LumaPlayerMicrophoneCapture::prepare(int32_t deviceIndex, int32_t queueDurationMs, int32_t deviceBufferMs)
{
    stop();
    const QStringList names = availableDeviceNames();
    if (deviceIndex < 0 || deviceIndex >= names.size())
    {
        m_error.store(LumaPlayerCoreCResultInvalidParam);
        return false;
    }
    const int32_t result = lumaPlayerCoreMicrophonePrepare(m_handle, deviceIndex, queueDurationMs,
        (std::max)(5, deviceBufferMs / 4));
    m_error.store(result);
    m_prepared.store(result == 0);
    if (result == 0)
    {
        m_deviceName = names[deviceIndex];
    }
    return result == 0;
}

bool LumaPlayerMicrophoneCapture::start(int32_t deviceIndex, int32_t queueDurationMs, int32_t deviceBufferMs)
{
    return prepare(deviceIndex, queueDurationMs, deviceBufferMs) && startPrepared();
}

bool LumaPlayerMicrophoneCapture::startPrepared()
{
    const int32_t result = lumaPlayerCoreMicrophoneStart(m_handle);
    m_error.store(result);
    if (result == 0)
    {
        m_generation.fetch_add(1);
    }
    return result == 0;
}

void LumaPlayerMicrophoneCapture::stop()
{
    lumaPlayerCoreMicrophoneStop(m_handle);
    m_prepared.store(false);
}

bool LumaPlayerMicrophoneCapture::takePcm(QByteArray* pcm, int32_t maxBytes, int32_t* remainingMs)
{
    if (pcm == nullptr || maxBytes < 2)
    {
        return false;
    }
    pcm->resize(maxBytes - maxBytes % 2);
    const size_t frames = lumaPlayerCoreMicrophoneRead(m_handle,
        reinterpret_cast<int16_t*>(pcm->data()), pcm->size() / 2, remainingMs);
    pcm->resize(static_cast<int32_t>(frames * 2));
    return frames != 0;
}

void LumaPlayerMicrophoneCapture::setControls(int32_t gain, bool muted, bool monitoring)
{
    lumaPlayerCoreMicrophoneControls(m_handle, gain, muted ? 1 : 0, monitoring ? 1 : 0);
}

LumaPlayerCoreCMicrophoneStats LumaPlayerMicrophoneCapture::stats() const
{
    LumaPlayerCoreCMicrophoneStats result = {};
    lumaPlayerCoreMicrophoneStats(m_handle, &result);
    return result;
}

bool LumaPlayerMicrophoneCapture::isRunning() const
{
    return stats().m_running != 0;
}

bool LumaPlayerMicrophoneCapture::hasOverflowed() const
{
    return stats().m_overflow != 0;
}

uint64_t LumaPlayerMicrophoneCapture::streamGeneration() const
{
    return m_generation.load();
}

int32_t LumaPlayerMicrophoneCapture::sampleRate() const
{
    return m_prepared.load() ? 48000 : 0;
}

int32_t LumaPlayerMicrophoneCapture::channels() const
{
    return m_prepared.load() ? 1 : 0;
}

QString LumaPlayerMicrophoneCapture::deviceName() const
{
    return m_deviceName;
}

QString LumaPlayerMicrophoneCapture::lastError() const
{
    const LumaPlayerCoreCMicrophoneStats state = stats();
    if (state.m_overflow != 0)
    {
        return QStringLiteral("麦克风录音队列溢出，请结束本次录音");
    }
    const int32_t error = m_error.load() != 0 ? m_error.load() : state.m_error;
    return error == 0 ? QString() : QString::fromStdWString(CStringManager::Format(L"麦克风设备错误 (%d)，请检查权限及设备连接", error));
}