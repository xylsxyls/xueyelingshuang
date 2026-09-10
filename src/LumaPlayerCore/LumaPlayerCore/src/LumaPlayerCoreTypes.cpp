#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerCoreTypes.h"
#include "LumaPlayerCoreConfig.h"

#include <climits>

LumaPlayerAudioFormat::LumaPlayerAudioFormat() :
m_sampleRate(0),
m_channels(0),
m_bitsPerSample(16)
{

}

bool LumaPlayerAudioFormat::isValid() const
{
	return m_sampleRate > 0 && m_channels > 0 && m_bitsPerSample > 0 && (m_bitsPerSample % 8) == 0 &&
		m_channels <= INT_MAX / (m_bitsPerSample / 8);
}

int32_t LumaPlayerAudioFormat::bytesPerSample() const
{
	if (!isValid())
	{
		return 0;
	}
	return m_bitsPerSample / 8;
}

int32_t LumaPlayerAudioFormat::bytesPerFrame() const
{
	int32_t bytes = bytesPerSample();
	if (bytes <= 0)
	{
		return 0;
	}
	return bytes * m_channels;
}

LumaPlayerVideoFormat::LumaPlayerVideoFormat() :
m_width(0),
m_height(0)
{

}

bool LumaPlayerVideoFormat::isValid() const
{
	return m_width > 0 && m_height > 0;
}

LumaPlayerVideoFrame::LumaPlayerVideoFrame() :
m_width(0),
m_height(0),
m_stride(0),
m_timestamp100ns(0),
m_duration100ns(LumaPlayerCoreConfig().m_defaultFrameDuration100ns),
m_frameIndex(-1),
m_keyFrame(false)
{

}

void LumaPlayerVideoFrame::reset()
{
	m_bgraPixels.clear();
	m_width = 0;
	m_height = 0;
	m_stride = 0;
	m_timestamp100ns = 0;
	m_duration100ns = LumaPlayerCoreConfig().m_defaultFrameDuration100ns;
	m_frameIndex = -1;
	m_keyFrame = false;
}

bool LumaPlayerVideoFrame::empty() const
{
	return m_bgraPixels.empty() || m_width <= 0 || m_height <= 0 || m_width > INT_MAX / 4 ||
		m_stride < m_width * 4 || static_cast<size_t>(m_stride) > m_bgraPixels.size() / static_cast<size_t>(m_height);
}

int64_t LumaPlayerVideoFrame::endTime100ns() const
{
	return m_duration100ns > 0 && m_timestamp100ns > INT64_MAX - m_duration100ns ?
		INT64_MAX : m_timestamp100ns + (m_duration100ns > 0 ? m_duration100ns : 0);
}

LumaPlayerAudioFrame::LumaPlayerAudioFrame() :
m_timestamp100ns(0),
m_duration100ns(0)
{

}

void LumaPlayerAudioFrame::reset()
{
	m_pcmData.clear();
	m_format = LumaPlayerAudioFormat();
	m_timestamp100ns = 0;
	m_duration100ns = 0;
}

bool LumaPlayerAudioFrame::empty() const
{
	return m_pcmData.empty() || !m_format.isValid() ||
		m_pcmData.size() % static_cast<size_t>(m_format.bytesPerFrame()) != 0;
}

int64_t LumaPlayerAudioFrame::endTime100ns() const
{
	return m_duration100ns > 0 && m_timestamp100ns > INT64_MAX - m_duration100ns ?
		INT64_MAX : m_timestamp100ns + (m_duration100ns > 0 ? m_duration100ns : 0);
}

LumaPlayerMediaInfo::LumaPlayerMediaInfo() :
m_duration100ns(0),
m_defaultVideoFrameDuration100ns(LumaPlayerCoreConfig().m_defaultFrameDuration100ns),
m_hasVideo(false),
m_hasAudio(false)
{

}

void LumaPlayerMediaInfo::reset()
{
	m_filePath.clear();
	m_duration100ns = 0;
	m_defaultVideoFrameDuration100ns = LumaPlayerCoreConfig().m_defaultFrameDuration100ns;
	m_videoFormat = LumaPlayerVideoFormat();
	m_audioFormat = LumaPlayerAudioFormat();
	m_hasVideo = false;
	m_hasAudio = false;
}

LumaPlayerLoopPointInfo::LumaPlayerLoopPointInfo() :
m_isSet(false),
m_frameStart100ns(0),
m_frameEnd100ns(0),
m_frameIndex(-1)
{

}

void LumaPlayerLoopPointInfo::reset()
{
	m_isSet = false;
	m_frameStart100ns = 0;
	m_frameEnd100ns = 0;
	m_frameIndex = -1;
}

LumaPlayerLoopRange::LumaPlayerLoopRange() :
m_aPoint(),
m_bPoint()
{

}

void LumaPlayerLoopRange::reset()
{
	m_aPoint.reset();
	m_bPoint.reset();
}

bool LumaPlayerLoopRange::isValid() const
{
	return m_aPoint.m_isSet && m_bPoint.m_isSet && m_bPoint.m_frameEnd100ns > m_aPoint.m_frameStart100ns;
}

int64_t LumaPlayerLoopRange::start100ns() const
{
	return m_aPoint.m_isSet ? m_aPoint.m_frameStart100ns : 0;
}

int64_t LumaPlayerLoopRange::end100ns() const
{
	return m_bPoint.m_isSet ? m_bPoint.m_frameEnd100ns : 0;
}

LumaPlayerSnapshot::LumaPlayerSnapshot() :
m_result(LumaPlayerCoreResultSuccess),
m_state(LumaPlayerCoreStateClosed),
m_mediaSerial(0),
m_position100ns(0),
m_ratePermille(LumaPlayerCoreConfig().m_defaultRatePermille),
m_enableAutoReplay(true)
{

}

void LumaPlayerSnapshot::reset()
{
	m_result = LumaPlayerCoreResultSuccess;
	m_state = LumaPlayerCoreStateClosed;
	m_mediaSerial = 0;
	m_mediaInfo.reset();
	m_position100ns = 0;
	m_ratePermille = LumaPlayerCoreConfig().m_defaultRatePermille;
	m_loopRange.reset();
	m_currentVideoFrame.reset();
	m_enableAutoReplay = true;
}