#include "KaraokePcmProcessor.h"

#include <algorithm>
#include <limits>

int16_t KaraokePcmProcessor::applyGain(int16_t sample, int32_t gainPercent)
{
    const int64_t scaled = static_cast<int64_t>(sample) * gainPercent / 100;
    const int64_t bounded = (std::max<int64_t>)((std::numeric_limits<int16_t>::min)(),
        (std::min<int64_t>)((std::numeric_limits<int16_t>::max)(), scaled));
    return static_cast<int16_t>(bounded);
}

bool KaraokePcmProcessor::processMicrophone(const int16_t* input, size_t sampleCount,
    int32_t gainPercent, bool muted, bool monitoring,
    int16_t* recording, int16_t* monitor)
{
    if (gainPercent < 0 || gainPercent > 5000 ||
        (sampleCount > 0 && (input == nullptr || recording == nullptr || monitor == nullptr)))
    {
        return false;
    }
    for (size_t i = 0; i < sampleCount; ++i)
    {
        const int16_t gained = muted ? 0 : applyGain(input[i], gainPercent);
        recording[i] = gained;
        monitor[i] = monitoring ? gained : 0;
    }
    return true;
}

bool KaraokePcmProcessor::mixS16(const int16_t* accompaniment,
    const int16_t* microphone, size_t sampleCount, int16_t* output)
{
    if (sampleCount > 0 &&
        (accompaniment == nullptr || microphone == nullptr || output == nullptr))
    {
        return false;
    }
    for (size_t i = 0; i < sampleCount; ++i)
    {
        const int32_t mixed = static_cast<int32_t>(accompaniment[i]) + microphone[i];
        const int32_t bounded = (std::max<int32_t>)((std::numeric_limits<int16_t>::min)(),
            (std::min<int32_t>)((std::numeric_limits<int16_t>::max)(), mixed));
        output[i] = static_cast<int16_t>(bounded);
    }
    return true;
}

bool KaraokePcmProcessor::convertMonitorS16(const int16_t* input, size_t inputFrames,
    int32_t inputRate, int32_t inputChannels, int32_t outputRate,
    int32_t outputChannels, uint64_t* frameRemainder,
    std::vector<int16_t>* output)
{
    if (output == nullptr || frameRemainder == nullptr || inputRate <= 0 ||
        outputRate <= 0 || inputChannels < 1 || inputChannels > 2 ||
        outputChannels < 1 || outputChannels > 2 ||
        (inputFrames > 0 && input == nullptr) ||
        *frameRemainder >= static_cast<uint64_t>(inputRate) ||
        inputFrames > ((std::numeric_limits<uint64_t>::max)() - *frameRemainder) /
                static_cast<uint64_t>(outputRate))
    {
        return false;
    }
    const uint64_t scaledFrames = static_cast<uint64_t>(inputFrames) *
            static_cast<uint64_t>(outputRate) +
        *frameRemainder;
    const uint64_t outputFrames = scaledFrames / static_cast<uint64_t>(inputRate);
    if (outputFrames > (std::numeric_limits<size_t>::max)() /
                static_cast<size_t>(outputChannels) ||
        outputFrames * static_cast<uint64_t>(outputChannels) > output->max_size())
    {
        return false;
    }
    std::vector<int16_t> converted;
    try
    {
        converted.resize(static_cast<size_t>(outputFrames) * outputChannels);
    }
    catch (...)
    {
        return false;
    }
    for (uint64_t frame = 0; frame < outputFrames; ++frame)
    {
        uint64_t sourceNumerator = frame * static_cast<uint64_t>(inputRate);
        uint64_t sourceFrame = sourceNumerator / static_cast<uint64_t>(outputRate);
        uint64_t fraction = sourceNumerator % static_cast<uint64_t>(outputRate);
        if (sourceFrame >= inputFrames)
        {
            sourceFrame = inputFrames == 0 ? 0 : inputFrames - 1;
            fraction = 0;
        }
        const uint64_t nextFrame = inputFrames == 0 || sourceFrame + 1 >= inputFrames
            ? sourceFrame
            : sourceFrame + 1;
        for (int32_t channel = 0; channel < outputChannels; ++channel)
        {
            int32_t sourceChannel = channel;
            if (inputChannels == 1)
            {
                sourceChannel = 0;
            }
            int32_t first = 0;
            int32_t second = 0;
            if (inputFrames > 0)
            {
                if (inputChannels == 2 && outputChannels == 1)
                {
                    first = (static_cast<int32_t>(input[sourceFrame * 2]) +
                                static_cast<int32_t>(input[sourceFrame * 2 + 1])) /
                        2;
                    second = (static_cast<int32_t>(input[nextFrame * 2]) +
                                 static_cast<int32_t>(input[nextFrame * 2 + 1])) /
                        2;
                }
                else
                {
                    first = input[sourceFrame * inputChannels + sourceChannel];
                    second = input[nextFrame * inputChannels + sourceChannel];
                }
            }
            const int64_t interpolated = static_cast<int64_t>(first) *
                    (static_cast<int64_t>(outputRate) - fraction) +
                static_cast<int64_t>(second) * fraction;
            converted[static_cast<size_t>(frame) * outputChannels + channel] =
                static_cast<int16_t>(interpolated / outputRate);
        }
    }
    output->swap(converted);
    *frameRemainder = scaledFrames % static_cast<uint64_t>(inputRate);
    return true;
}