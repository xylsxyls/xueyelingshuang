#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "PlayerEngineHelper.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>


int64_t PlayerEngineHelper::clampInt64(int64_t value, int64_t minValue, int64_t maxValue)
{
	if (value < minValue)
	{
		return minValue;
	}
	if (value > maxValue)
	{
		return maxValue;
	}
	return value;
}

int32_t PlayerEngineHelper::clampRate(int32_t ratePermille, int32_t minRatePermille, int32_t maxRatePermille)
{
	if (ratePermille < minRatePermille)
	{
		return minRatePermille;
	}
	if (ratePermille > maxRatePermille)
	{
		return maxRatePermille;
	}
	return ratePermille;
}

void PlayerEngineHelper::deleteReader(FFmpegCppPlaybackReader** reader)
{
	if (reader == nullptr || *reader == nullptr)
	{
		return;
	}
	delete *reader;
	*reader = nullptr;
}

void PlayerEngineHelper::convertVideoFrame(FFmpegCppPlaybackVideoFrame* source, int64_t frameIndex, LumaPlayerVideoFrame* target)
{
	if (source == nullptr || target == nullptr)
	{
		return;
	}
	target->reset();
	target->m_bgraPixels.swap(source->bgraPixels);
	target->m_width = source->width;
	target->m_height = source->height;
	target->m_stride = source->stride;
	target->m_timestamp100ns = source->timestamp100ns;
	target->m_duration100ns = source->duration100ns;
	target->m_keyFrame = source->keyFrame;
	target->m_frameIndex = frameIndex;
}

void PlayerEngineHelper::convertAudioFrame(FFmpegCppPlaybackAudioFrame* source, LumaPlayerAudioFrame* target)
{
	if (source == nullptr || target == nullptr)
	{
		return;
	}
	target->reset();
	target->m_pcmData.swap(source->pcmData);
	target->m_format.m_sampleRate = source->sampleRate;
	target->m_format.m_channels = source->channels;
	target->m_format.m_bitsPerSample = source->bitsPerSample;
	target->m_timestamp100ns = source->timestamp100ns;
	target->m_duration100ns = source->duration100ns;
}

bool PlayerEngineHelper::scaleAudioRate(const LumaPlayerAudioFrame& source, int32_t ratePermille, LumaPlayerAudioFrame* target)
{
    if (target == nullptr || target == &source || source.empty() ||
        source.m_format.m_bitsPerSample != 16 || ratePermille < 50 || ratePermille > 20000)
    {
        return false;
    }
    const size_t bytesPerFrame = static_cast<size_t>(source.m_format.bytesPerFrame());
    if (bytesPerFrame == 0 || source.m_pcmData.size() % bytesPerFrame != 0)
    {
        return false;
    }
    FFmpegCppAudioTempo tempo;
    if (!tempo.init(source.m_format.m_sampleRate, source.m_format.m_channels) || !tempo.setRate(ratePermille))
    {
        return false;
    }
    std::vector<int16_t> input(source.m_pcmData.size() / sizeof(int16_t));
    std::memcpy(&input[0], &source.m_pcmData[0], source.m_pcmData.size());
    if (!tempo.write(&input[0], source.m_pcmData.size() / bytesPerFrame) || !tempo.finish())
    {
        return false;
    }
    const size_t frames = tempo.available();
    if (frames == 0)
    {
        return false;
    }
    std::vector<int16_t> output(frames * source.m_format.m_channels);
    if (tempo.read(&output[0], frames) != frames)
    {
        return false;
    }
    target->reset();
    target->m_format = source.m_format;
    target->m_timestamp100ns = source.m_timestamp100ns;
    target->m_duration100ns = source.m_duration100ns;
    target->m_pcmData.resize(output.size() * sizeof(int16_t));
    std::memcpy(&target->m_pcmData[0], &output[0], target->m_pcmData.size());
    return true;
}