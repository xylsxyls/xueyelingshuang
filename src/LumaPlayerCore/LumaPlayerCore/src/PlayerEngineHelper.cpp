#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "PlayerEngineHelper.h"

#include <cstring>
#include <limits>

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
	if (target == nullptr || target == &source || ratePermille <= 0 ||
		source.empty() || source.m_format.m_bitsPerSample != 16)
	{
		return false;
	}
	const size_t channels = static_cast<size_t>(source.m_format.m_channels);
	const size_t bytesPerFrame = channels * sizeof(int16_t);
	if (bytesPerFrame == 0 || source.m_pcmData.size() % bytesPerFrame != 0)
	{
		return false;
	}
	const size_t inputFrames = source.m_pcmData.size() / bytesPerFrame;
	if (inputFrames > ((std::numeric_limits<uint64_t>::max)() - ratePermille / 2) / 1000)
	{
		return false;
	}
	const uint64_t scaledFrames = (static_cast<uint64_t>(inputFrames) * 1000 + ratePermille / 2) / ratePermille;
	const uint64_t outputFrames64 = scaledFrames > 0 ? scaledFrames : 1;
	if (outputFrames64 == 0 || outputFrames64 > (std::numeric_limits<size_t>::max)() / bytesPerFrame)
	{
		return false;
	}
	const size_t outputFrames = static_cast<size_t>(outputFrames64);
	target->reset();
	target->m_format = source.m_format;
	target->m_timestamp100ns = source.m_timestamp100ns;
	target->m_duration100ns = source.m_duration100ns;
	target->m_pcmData.resize(outputFrames * bytesPerFrame);
	for (size_t index = 0; index < outputFrames; ++index)
	{
		const double position = static_cast<double>(index) * inputFrames / outputFrames;
		const size_t left = static_cast<size_t>(position);
		const size_t right = left + 1 < inputFrames ? left + 1 : left;
		const double fraction = position - left;
		for (size_t channel = 0; channel < channels; ++channel)
		{
			int16_t first = 0;
			int16_t second = 0;
			std::memcpy(&first, &source.m_pcmData[left * bytesPerFrame + channel * sizeof(int16_t)], sizeof(first));
			std::memcpy(&second, &source.m_pcmData[right * bytesPerFrame + channel * sizeof(int16_t)], sizeof(second));
			const int16_t value = static_cast<int16_t>(first + (second - first) * fraction);
			std::memcpy(&target->m_pcmData[index * bytesPerFrame + channel * sizeof(int16_t)], &value, sizeof(value));
		}
	}
	return true;
}