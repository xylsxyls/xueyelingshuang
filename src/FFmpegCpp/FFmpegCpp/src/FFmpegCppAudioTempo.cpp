#include "FFmpegCppAudioTempo.h"
#include "soundtouchinclude/SoundTouch.h"
#include "soundtouchinclude/SoundTouchDLL.h"
#include <climits>
#include <cmath>
#include <algorithm>
#include <vector>

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
    try
    {
        m_stream = soundtouch_createInstance();
        if (m_stream == nullptr ||
            !soundtouch_setSampleRate(m_stream, sampleRate) ||
            !soundtouch_setChannels(m_stream, channels) ||
            !soundtouch_setSetting(m_stream, SETTING_USE_QUICKSEEK, 0) ||
            !soundtouch_setSetting(m_stream, SETTING_USE_AA_FILTER, 1) ||
            !soundtouch_setSetting(m_stream, SETTING_AA_FILTER_LENGTH, 64))
        {
            uninit();
            return false;
        }
        m_channels = channels;
    }
    catch (...)
    {
        uninit();
    }
    return m_stream != nullptr;
}

void FFmpegCppAudioTempo::uninit()
{
    if (m_stream != nullptr)
    {
        soundtouch_destroyInstance(m_stream);
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
    try
    {
        soundtouch_setTempo(m_stream, ratePermille / 1000.0f);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool FFmpegCppAudioTempo::setPitchSemitones(int32_t semitones)
{
    if (m_stream == nullptr || semitones < -7 || semitones > 7)
    {
        return false;
    }
    const float pitch = static_cast<float>(std::pow(2.0,
        static_cast<double>(semitones) / 12.0));
    try
    {
        soundtouch_setPitch(m_stream, pitch);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool FFmpegCppAudioTempo::write(const int16_t* samples, size_t frames)
{
    if (m_stream == nullptr || samples == nullptr || m_channels <= 0 ||
        frames > static_cast<size_t>(INT_MAX / m_channels / sizeof(float)))
    {
        return false;
    }
    try
    {
        std::vector<float> input(frames * m_channels);
        for (size_t i = 0; i < input.size(); ++i)
        {
            input[i] = samples[i] / 32768.0f;
        }
        return soundtouch_putSamples(m_stream, input.data(), static_cast<unsigned int>(frames)) != 0;
    }
    catch (...)
    {
        return false;
    }
}

bool FFmpegCppAudioTempo::finish()
{
    if (m_stream == nullptr)
    {
        return false;
    }
    try
    {
        return soundtouch_flush(m_stream) != 0;
    }
    catch (...)
    {
        return false;
    }
}

size_t FFmpegCppAudioTempo::available() const
{
    return m_stream == nullptr ? 0 : soundtouch_numSamples(m_stream);
}

size_t FFmpegCppAudioTempo::read(int16_t* samples, size_t frames)
{
    if (m_stream == nullptr || samples == nullptr || m_channels <= 0 ||
        frames > static_cast<size_t>(INT_MAX / m_channels / sizeof(float)))
    {
        return 0;
    }
    try
    {
        std::vector<float> output((std::min)(frames, available()) * m_channels);
        if (output.empty())
        {
            return 0;
        }
        const size_t count = soundtouch_receiveSamples(m_stream, output.data(),
            static_cast<unsigned int>(output.size() / m_channels));
        for (size_t i = 0; i < count * m_channels; ++i)
        {
            if (!std::isfinite(output[i]))
            {
                return 0;
            }
            samples[i] = static_cast<int16_t>((std::max)(-32768.0f, (std::min)(32767.0f, output[i] * 32768.0f)));
        }
        return count;
    }
    catch (...)
    {
        return 0;
    }
}