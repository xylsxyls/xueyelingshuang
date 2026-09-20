#include "FFmpegCppAudioTempo.h"
#include "sonicinclude/sonic.h"
#include <climits>

FFmpegCppAudioTempo::FFmpegCppAudioTempo() :
m_stream(nullptr),
m_channels(0)
{

}

FFmpegCppAudioTempo::~FFmpegCppAudioTempo()
{
    uninit();
}

bool FFmpegCppAudioTempo::init(int32_t sampleRate, int32_t channels)
{
    uninit();
    if (sampleRate < 1000 || sampleRate > 500000 || channels < 1 || channels > 32)
    {
        return false;
    }
    m_stream = sonicCreateStream(sampleRate, channels);
    m_channels = m_stream != nullptr ? channels : 0;
    return m_stream != nullptr;
}

void FFmpegCppAudioTempo::uninit()
{
    if (m_stream != nullptr)
    {
        sonicDestroyStream(static_cast<sonicStream>(m_stream));
        m_stream = nullptr;
    }
    m_channels = 0;
}

bool FFmpegCppAudioTempo::setRate(int32_t ratePermille)
{
    if (m_stream == nullptr || ratePermille < 50 || ratePermille > 20000)
    {
        return false;
    }
    sonicSetSpeed(static_cast<sonicStream>(m_stream), ratePermille / 1000.0f);
    return true;
}

bool FFmpegCppAudioTempo::write(const int16_t* samples, size_t frames)
{
    return m_stream != nullptr && samples != nullptr && m_channels > 0 &&
        frames <= static_cast<size_t>(INT_MAX / m_channels / sizeof(int16_t)) &&
        sonicWriteShortToStream(static_cast<sonicStream>(m_stream), const_cast<int16_t*>(samples), static_cast<int>(frames)) != 0;
}

bool FFmpegCppAudioTempo::finish()
{
    return m_stream != nullptr && sonicFlushStream(static_cast<sonicStream>(m_stream)) != 0;
}

size_t FFmpegCppAudioTempo::available() const
{
    return m_stream == nullptr ? 0 : static_cast<size_t>(sonicSamplesAvailable(static_cast<sonicStream>(m_stream)));
}

size_t FFmpegCppAudioTempo::read(int16_t* samples, size_t frames)
{
    if (m_stream == nullptr || samples == nullptr || m_channels <= 0 ||
        frames > static_cast<size_t>(INT_MAX / m_channels / sizeof(int16_t)))
    {
        return 0;
    }
    return static_cast<size_t>(sonicReadShortFromStream(static_cast<sonicStream>(m_stream), samples, static_cast<int>(frames)));
}