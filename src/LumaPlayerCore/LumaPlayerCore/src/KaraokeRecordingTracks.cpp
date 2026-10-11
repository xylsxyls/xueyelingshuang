#include "KaraokeRecordingTracks.h"

#include <algorithm>
#include <limits>

bool KaraokeRecordingTracks::init(int32_t rate, int32_t channels, size_t bytes)
{
    KaraokeRecordingTracks replacement;
    for (int i = 0; i < 3; ++i)
    {
        if (!replacement.m_tracks[i].init(rate, channels, bytes))
        {
            return false;
        }
    }
    *this = replacement;
    return true;
}

void KaraokeRecordingTracks::clear()
{
    for (int i = 0; i < 3; ++i)
    {
        m_tracks[i].clear();
    }
}

bool KaraokeRecordingTracks::configureSong(int64_t duration, int32_t rate)
{
    KaraokeRecordingTracks replacement(*this);
    for (int i = 0; i < 3; ++i)
    {
        if (!replacement.m_tracks[i].configureSong(duration, rate))
        {
            return false;
        }
    }
    *this = replacement;
    return true;
}

int32_t KaraokeRecordingTracks::recordingRate() const
{
    for (int i = 0; i < 3; ++i)
    {
        if (m_tracks[i].capturedFrameCount())
        {
            return m_tracks[i].recordingRate();
        }
    }
    return m_tracks[0].recordingRate();
}
int32_t KaraokeRecordingTracks::sampleRate() const
{
    return m_tracks[0].sampleRate();
}
int32_t KaraokeRecordingTracks::channels() const
{
    return m_tracks[0].channels();
}

uint64_t KaraokeRecordingTracks::capturedFrameCount() const
{
    uint64_t count = 0;
    for (int i = 0; i < 3; ++i)
    {
        count += m_tracks[i].capturedFrameCount();
    }
    return count;
}

bool KaraokeRecordingTracks::advanceTo(int64_t position)
{
    if (position < 0)
    {
        return false;
    }
    for (int i = 0; i < 3; ++i)
    {
        if (!m_tracks[i].advanceTo(position))
        {
            return false;
        }
    }
    return true;
}

bool KaraokeRecordingTracks::replaceCapturedRange(int64_t start, int64_t end,
    int32_t rate, const int16_t* pcm, size_t frames, int32_t microphone)
{
    return microphone >= 0 && microphone < 3 &&
        m_tracks[microphone].replaceCapturedRange(start, end, rate, pcm, frames);
}

bool KaraokeRecordingTracks::outputFrameCount(uint64_t* frames) const
{
    if (!frames)
    {
        return false;
    }
    *frames = 0;
    for (int i = 0; i < 3; ++i)
    {
        uint64_t count = 0;
        if (!m_tracks[i].outputFrameCount(&count))
        {
            return false;
        }
        *frames = (std::max)(*frames, count);
    }
    return true;
}

std::vector<KaraokeTimelineSegment> KaraokeRecordingTracks::outputSegments() const
{
    return m_tracks[0].outputSegments();
}

bool KaraokeRecordingTracks::renderOutputFrames(uint64_t start, size_t frames, int16_t* output) const
{
    uint64_t total = 0;
    if (channels() <= 0 || !outputFrameCount(&total) || start > total ||
        frames > total - start || (frames && !output) ||
        frames > (std::numeric_limits<size_t>::max)() / channels())
    {
        return false;
    }
    const size_t samples = frames * channels();
    std::vector<int32_t> mix(samples, 0);
    std::vector<int16_t> scratch(samples, 0);
    for (int i = 0; i < 3; ++i)
    {
        uint64_t count = 0;
        if (!m_tracks[i].outputFrameCount(&count))
        {
            return false;
        }
        const size_t used = start < count ? static_cast<size_t>((std::min<uint64_t>)(frames, count - start)) : 0;
        if (used && !m_tracks[i].renderOutputFrames(start, used, scratch.data()))
        {
            return false;
        }
        for (size_t n = 0; n < used * channels(); ++n)
        {
            mix[n] += scratch[n];
        }
    }
    // Saturate only after summing all lanes, so opposite polarity is not lost.
    for (size_t n = 0; n < samples; ++n)
    {
        output[n] = static_cast<int16_t>((std::max)(-32768, (std::min)(32767, mix[n])));
    }
    return true;
}