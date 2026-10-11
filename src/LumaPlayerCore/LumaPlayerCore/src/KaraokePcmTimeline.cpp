#include "KaraokePcmTimeline.h"

#include <algorithm>
#include <limits>
#include <set>

KaraokePcmTimeline::KaraokePcmTimeline() :
m_sampleRate(0),
m_songDuration100ns(0),
m_songRate(0),
m_channels(0),
m_maxPcmBytes(0)
{

}

bool KaraokePcmTimeline::init(int32_t sampleRate, int32_t channels,
    size_t maxPcmBytes)
{
    if (sampleRate < 1 || sampleRate > 500000 || channels < 1 ||
        channels > 32)
    {
        return false;
    }
    clear();
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_maxPcmBytes = maxPcmBytes;
    return true;
}

void KaraokePcmTimeline::clear()
{
    m_songDuration100ns = 0;
    m_songRate = 0;
    m_timeline.clear();
    m_capturedRanges.clear();
    m_store.reset();
}

bool KaraokePcmTimeline::advanceTo(int64_t mediaPosition100ns)
{
    return m_sampleRate > 0 && m_timeline.advanceTo(mediaPosition100ns);
}

size_t KaraokePcmTimeline::frameOffsetForMediaPosition(
    const KaraokePcmCapturedRange& range, int64_t mediaPosition100ns)
{
    if (mediaPosition100ns <= range.m_mediaStart100ns)
    {
        return 0;
    }
    if (mediaPosition100ns >= range.m_mediaEnd100ns)
    {
        return range.m_frameCount;
    }
    const long double ratio = static_cast<long double>(mediaPosition100ns - range.m_mediaStart100ns) /
        static_cast<long double>(range.m_mediaEnd100ns - range.m_mediaStart100ns);
    size_t offset = static_cast<size_t>(ratio * range.m_frameCount + 0.5L);
    if (offset > range.m_frameCount)
    {
        offset = range.m_frameCount;
    }
    return offset;
}

uint64_t KaraokePcmTimeline::silenceFrameCount(int64_t duration100ns,
    int32_t sampleRate)
{
    if (duration100ns <= 0 || sampleRate <= 0)
    {
        return 0;
    }
    const long double frames = static_cast<long double>(duration100ns) * sampleRate / 10000000.0L;
    if (frames >= static_cast<long double>((std::numeric_limits<uint64_t>::max)()))
    {
        return (std::numeric_limits<uint64_t>::max)();
    }
    return static_cast<uint64_t>(frames + 0.5L);
}

bool KaraokePcmTimeline::storedPcmBytes(
    const std::vector<KaraokePcmCapturedRange>& ranges, size_t* byteCount) const
{
    if (byteCount == nullptr)
    {
        return false;
    }
    std::set<const std::vector<int16_t>*> blocks;
    size_t totalBytes = 0;
    try
    {
        for (size_t i = 0; i < ranges.size(); ++i)
        {
            if (!ranges[i].m_pcm || !blocks.insert(ranges[i].m_pcm.get()).second)
            {
                continue;
            }
            const size_t sampleCount = ranges[i].m_pcm->size();
            if (sampleCount > (std::numeric_limits<size_t>::max)() / sizeof(int16_t))
            {
                return false;
            }
            const size_t blockBytes = sampleCount * sizeof(int16_t);
            if (totalBytes > (std::numeric_limits<size_t>::max)() - blockBytes)
            {
                return false;
            }
            totalBytes += blockBytes;
        }
    }
    catch (...)
    {
        return false;
    }
    *byteCount = totalBytes;
    return true;
}

bool KaraokePcmTimeline::replaceCapturedRange(int64_t mediaStart100ns,
    int64_t mediaEnd100ns, int32_t ratePermille,
    const int16_t* interleavedPcm, size_t frameCount)
{
    if (m_songRate != 0 && ratePermille != m_songRate)
    {
        return false;
    }
    if (m_sampleRate <= 0 || m_channels <= 0 ||
        mediaStart100ns < 0 || mediaEnd100ns <= mediaStart100ns ||
        ratePermille < 100 || ratePermille > 5000 ||
        interleavedPcm == nullptr || frameCount == 0 ||
        frameCount > (std::numeric_limits<size_t>::max)() / static_cast<size_t>(m_channels))
    {
        return false;
    }
    const size_t sampleCount = frameCount * static_cast<size_t>(m_channels);
    if (sampleCount > (std::numeric_limits<size_t>::max)() / sizeof(int16_t))
    {
        return false;
    }
    const size_t newBlockBytes = sampleCount * sizeof(int16_t);
    size_t currentBytes = 0;
    if (m_maxPcmBytes != 0 && (!storedPcmBytes(m_capturedRanges, &currentBytes) || currentBytes > m_maxPcmBytes || newBlockBytes > m_maxPcmBytes - currentBytes))
    {
        return false;
    }

    try
    {
        if (m_maxPcmBytes == 0 && !m_store)
        {
            m_store.reset(new KaraokeStorage);
        }
        if (m_store && !m_store->valid())
        {
            m_store.reset();
            return false;
        }
        // Continuous capture uses one disk span, not a growing list of 10 ms allocations.
        if (m_store && !m_capturedRanges.empty())
        {
            KaraokePcmCapturedRange& tail = m_capturedRanges.back();
            if (tail.m_mediaEnd100ns == mediaStart100ns && tail.m_ratePermille == ratePermille &&
                tail.m_store == m_store && tail.m_storeOffset + (tail.m_sampleOffsetFrames + tail.m_frameCount) * static_cast<uint64_t>(m_channels) * 2 == m_store->size())
            {
                KaraokeTimeline next = m_timeline;
                if (!next.replaceCapturedRange(mediaStart100ns, mediaEnd100ns, ratePermille))
                {
                    return false;
                }
                if (m_store->append(interleavedPcm, newBlockBytes) == UINT64_MAX)
                {
                    return false;
                }
                tail.m_mediaEnd100ns = mediaEnd100ns;
                tail.m_frameCount += frameCount;
                m_timeline.swap(next);
                return true;
            }
        }
        KaraokeTimeline nextTimeline = m_timeline;
        if (!nextTimeline.replaceCapturedRange(mediaStart100ns, mediaEnd100ns, ratePermille))
        {
            return false;
        }

        std::vector<KaraokePcmCapturedRange> nextRanges;
        nextRanges.reserve(m_capturedRanges.size() + 2);
        for (size_t i = 0; i < m_capturedRanges.size(); ++i)
        {
            const KaraokePcmCapturedRange& current = m_capturedRanges[i];
            if (current.m_mediaEnd100ns <= mediaStart100ns ||
                current.m_mediaStart100ns >= mediaEnd100ns)
            {
                nextRanges.push_back(current);
                continue;
            }

            if (current.m_mediaStart100ns < mediaStart100ns)
            {
                const size_t leftFrames = frameOffsetForMediaPosition(current, mediaStart100ns);
                if (leftFrames > 0)
                {
                    KaraokePcmCapturedRange left = current;
                    left.m_mediaEnd100ns = mediaStart100ns;
                    left.m_frameCount = leftFrames;
                    nextRanges.push_back(left);
                }
            }

            if (current.m_mediaEnd100ns > mediaEnd100ns)
            {
                const size_t rightOffset = frameOffsetForMediaPosition(current, mediaEnd100ns);
                if (rightOffset < current.m_frameCount)
                {
                    KaraokePcmCapturedRange right = current;
                    right.m_mediaStart100ns = mediaEnd100ns;
                    right.m_sampleOffsetFrames += rightOffset;
                    right.m_frameCount -= rightOffset;
                    nextRanges.push_back(right);
                }
            }
        }

        std::shared_ptr<std::vector<int16_t>> block;
        uint64_t diskOffset = 0;
        if (m_store)
        {
            diskOffset = m_store->append(interleavedPcm, newBlockBytes);
            if (diskOffset == UINT64_MAX)
            {
                return false;
            }
        }
        else
        {
            block.reset(new std::vector<int16_t>(interleavedPcm, interleavedPcm + sampleCount));
        }
        KaraokePcmCapturedRange captured;
        captured.m_mediaStart100ns = mediaStart100ns;
        captured.m_mediaEnd100ns = mediaEnd100ns;
        captured.m_ratePermille = ratePermille;
        captured.m_pcm = block;
        captured.m_store = m_store;
        captured.m_storeOffset = diskOffset;
        captured.m_sampleOffsetFrames = 0;
        captured.m_frameCount = frameCount;
        nextRanges.push_back(captured);
        /** 按原媒体起点排序保留的PCM区间
        @param [in] left 借用的左侧候选区间
        @param [in] right 借用的右侧候选区间
        @return 左侧起点更早时返回true
        */
        std::sort(nextRanges.begin(), nextRanges.end(),
            [](const KaraokePcmCapturedRange& left, const KaraokePcmCapturedRange& right)
            {
                return left.m_mediaStart100ns < right.m_mediaStart100ns;
            });

        size_t nextBytes = 0;
        if (m_maxPcmBytes != 0 && (!storedPcmBytes(nextRanges, &nextBytes) || nextBytes > m_maxPcmBytes))
        {
            return false;
        }
        m_timeline.swap(nextTimeline);
        m_capturedRanges.swap(nextRanges);
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool KaraokePcmTimeline::rangeFrameCount(uint64_t* frameCount) const
{
    if (frameCount == nullptr || m_sampleRate <= 0)
    {
        return false;
    }
    uint64_t totalFrames = 0;
    int64_t mediaCursor = 0;
    for (size_t i = 0; i < m_capturedRanges.size(); ++i)
    {
        const KaraokePcmCapturedRange& range = m_capturedRanges[i];
        if (range.m_mediaStart100ns < mediaCursor || (!range.m_pcm && !range.m_store) ||
            (range.m_pcm && (range.m_sampleOffsetFrames > range.m_pcm->size() / static_cast<size_t>(m_channels) || range.m_frameCount > range.m_pcm->size() / static_cast<size_t>(m_channels) - range.m_sampleOffsetFrames)))
        {
            return false;
        }
        const uint64_t gapFrames = silenceFrameCount(range.m_mediaStart100ns - mediaCursor, m_sampleRate);
        if (gapFrames == (std::numeric_limits<uint64_t>::max)() ||
            totalFrames > (std::numeric_limits<uint64_t>::max)() - gapFrames ||
            totalFrames + gapFrames > (std::numeric_limits<uint64_t>::max)() - range.m_frameCount)
        {
            return false;
        }
        totalFrames += gapFrames + range.m_frameCount;
        mediaCursor = range.m_mediaEnd100ns;
    }
    const int64_t mediaDuration100ns = m_timeline.mediaDuration100ns();
    if (mediaCursor > mediaDuration100ns)
    {
        return false;
    }
    const uint64_t trailingFrames = silenceFrameCount(mediaDuration100ns - mediaCursor, m_sampleRate);
    if (trailingFrames == (std::numeric_limits<uint64_t>::max)() ||
        totalFrames > (std::numeric_limits<uint64_t>::max)() - trailingFrames)
    {
        return false;
    }
    *frameCount = totalFrames + trailingFrames;
    return true;
}

bool KaraokePcmTimeline::outputFrameCount(uint64_t* frameCount) const
{
    if (m_songRate != 0 && frameCount != nullptr)
    {
        *frameCount = songFrame(m_songDuration100ns);
        return true;
    }
    return rangeFrameCount(frameCount);
}

std::vector<KaraokeTimelineSegment> KaraokePcmTimeline::outputSegments() const
{
    std::vector<KaraokeTimelineSegment> result;
    if (m_songRate != 0)
    {
        KaraokeTimelineSegment segment = {0, m_songDuration100ns, 0,
            static_cast<int64_t>(songFrame(m_songDuration100ns) * 10000000 / m_sampleRate), m_songRate, false};
        result.push_back(segment);
        return result;
    }
    uint64_t total = 0;
    if (!rangeFrameCount(&total) || total > static_cast<uint64_t>(INT64_MAX) / 10000000)
    {
        return result;
    }
    uint64_t cursor = 0;
    int64_t media = 0;
    for (size_t i = 0; i <= m_capturedRanges.size(); ++i)
    {
        const int64_t start = i < m_capturedRanges.size() ? m_capturedRanges[i].m_mediaStart100ns : m_timeline.mediaDuration100ns();
        const uint64_t gap = silenceFrameCount(start - media, m_sampleRate);
        if (gap)
        {
            KaraokeTimelineSegment segment = {media, start,
                static_cast<int64_t>((cursor * 10000000 + m_sampleRate - 1) / m_sampleRate),
                static_cast<int64_t>(((cursor + gap) * 10000000 + m_sampleRate - 1) / m_sampleRate), 1000, false};
            result.push_back(segment);
            cursor += gap;
        }
        if (i == m_capturedRanges.size())
        {
            break;
        }
        const KaraokePcmCapturedRange& range = m_capturedRanges[i];
        KaraokeTimelineSegment segment = {start, range.m_mediaEnd100ns,
            static_cast<int64_t>((cursor * 10000000 + m_sampleRate - 1) / m_sampleRate),
            static_cast<int64_t>(((cursor + range.m_frameCount) * 10000000 + m_sampleRate - 1) / m_sampleRate),
            range.m_ratePermille, true};
        result.push_back(segment);
        cursor += range.m_frameCount;
        media = range.m_mediaEnd100ns;
    }
    return result;
}

bool KaraokePcmTimeline::renderOutputFrames(uint64_t startFrame,
    size_t frameCount, int16_t* interleavedOutput) const
{
    if (m_sampleRate <= 0 || m_channels <= 0 || interleavedOutput == nullptr ||
        frameCount > (std::numeric_limits<size_t>::max)() / static_cast<size_t>(m_channels))
    {
        return false;
    }
    if (m_songRate != 0)
    {
        return renderSongFrames(startFrame, frameCount, interleavedOutput);
    }
    uint64_t totalFrames = 0;
    if (!rangeFrameCount(&totalFrames) || startFrame > totalFrames ||
        static_cast<uint64_t>(frameCount) > totalFrames - startFrame)
    {
        return false;
    }
    std::fill(interleavedOutput,
        interleavedOutput + frameCount * static_cast<size_t>(m_channels), 0);
    if (frameCount == 0)
    {
        return true;
    }

    const uint64_t requestedEnd = startFrame + static_cast<uint64_t>(frameCount);
    uint64_t outputCursor = 0;
    int64_t mediaCursor = 0;
    for (size_t i = 0; i < m_capturedRanges.size(); ++i)
    {
        const KaraokePcmCapturedRange& range = m_capturedRanges[i];
        const uint64_t gapFrames = silenceFrameCount(range.m_mediaStart100ns - mediaCursor, m_sampleRate);
        if (gapFrames > 0)
        {
            outputCursor += gapFrames;
        }
        const uint64_t segmentStart = outputCursor;
        const uint64_t segmentEnd = segmentStart + range.m_frameCount;
        const uint64_t copyStart = (std::max)(startFrame, segmentStart);
        const uint64_t copyEnd = (std::min)(requestedEnd, segmentEnd);
        if (copyStart < copyEnd)
        {
            const size_t sourceFrame = range.m_sampleOffsetFrames +
                static_cast<size_t>(copyStart - segmentStart);
            const size_t destinationFrame = static_cast<size_t>(copyStart - startFrame);
            const size_t copyFrames = static_cast<size_t>(copyEnd - copyStart);
            const size_t sampleOffset = sourceFrame * static_cast<size_t>(m_channels);
            const size_t destinationOffset = destinationFrame * static_cast<size_t>(m_channels);
            const size_t copySamples = copyFrames * static_cast<size_t>(m_channels);
            if (range.m_store)
            {
                if (!range.m_store->read(range.m_storeOffset + sampleOffset * sizeof(int16_t),
                        interleavedOutput + destinationOffset, copySamples * sizeof(int16_t)))
                {
                    return false;
                }
            }
            else
            {
                std::copy(range.m_pcm->begin() + sampleOffset,
                    range.m_pcm->begin() + sampleOffset + copySamples,
                    interleavedOutput + destinationOffset);
            }
        }
        outputCursor = segmentEnd;
        mediaCursor = range.m_mediaEnd100ns;
    }

    const uint64_t trailingFrames = silenceFrameCount(
        m_timeline.mediaDuration100ns() - mediaCursor, m_sampleRate);
    if (trailingFrames > 0)
    {
        outputCursor += trailingFrames;
    }
    return outputCursor == totalFrames;
}

size_t KaraokePcmTimeline::storedPcmBytes() const
{
    size_t byteCount = 0;
    return storedPcmBytes(m_capturedRanges, &byteCount) ? byteCount : 0;
}

int32_t KaraokePcmTimeline::sampleRate() const
{
    return m_sampleRate;
}

int32_t KaraokePcmTimeline::channels() const
{
    return m_channels;
}

const KaraokeTimeline& KaraokePcmTimeline::timeline() const
{
    return m_timeline;
}

bool KaraokePcmTimeline::configureSong(int64_t duration100ns, int32_t ratePermille)
{
    if (m_sampleRate <= 0 || duration100ns <= 0 || duration100ns > INT64_MAX / m_sampleRate ||
        ratePermille < 100 || ratePermille > 5000)
    {
        return false;
    }
    if (static_cast<long double>(duration100ns) * m_sampleRate / (10000.0L * ratePermille) >=
        static_cast<long double>(INT64_MAX / 10000000))
    {
        return false;
    }
    for (size_t i = 0; i < m_capturedRanges.size(); ++i)
    {
        if (m_capturedRanges[i].m_ratePermille != ratePermille)
        {
            return false;
        }
    }
    m_songDuration100ns = duration100ns;
    m_songRate = ratePermille;
    return true;
}

int32_t KaraokePcmTimeline::recordingRate() const
{
    return m_songRate != 0 ? m_songRate : (m_capturedRanges.empty() ? 1000 : m_capturedRanges.front().m_ratePermille);
}

uint64_t KaraokePcmTimeline::capturedFrameCount() const
{
    uint64_t total = 0;
    for (size_t i = 0; i < m_capturedRanges.size(); ++i)
    {
        total += m_capturedRanges[i].m_frameCount;
    }
    return total;
}

uint64_t KaraokePcmTimeline::songFrame(int64_t position100ns) const
{
    const int64_t position = (std::max<int64_t>)(0, (std::min)(position100ns, m_songDuration100ns));
    return static_cast<uint64_t>(static_cast<long double>(position) * m_sampleRate /
            (10000.0L * m_songRate) +
        0.5L);
}

bool KaraokePcmTimeline::renderSongFrames(uint64_t startFrame, size_t frameCount, int16_t* output) const
{
    const uint64_t total = songFrame(m_songDuration100ns);
    if (startFrame > total || frameCount > total - startFrame)
    {
        return false;
    }
    std::fill(output, output + frameCount * m_channels, 0);
    for (size_t i = 0; i < m_capturedRanges.size(); ++i)
    {
        const KaraokePcmCapturedRange& range = m_capturedRanges[i];
        const uint64_t first = songFrame(range.m_mediaStart100ns);
        const uint64_t last = (std::min)(songFrame(range.m_mediaEnd100ns), first + range.m_frameCount);
        const uint64_t begin = (std::max)(first, startFrame);
        const uint64_t end = (std::min)(last, startFrame + frameCount);
        if (end <= begin)
        {
            continue;
        }
        const size_t offset = (range.m_sampleOffsetFrames + static_cast<size_t>(begin - first)) * m_channels;
        const size_t count = static_cast<size_t>(end - begin) * m_channels;
        int16_t* target = output + static_cast<size_t>(begin - startFrame) * m_channels;
        if (range.m_store)
        {
            if (!range.m_store->read(range.m_storeOffset + offset * sizeof(int16_t), target, count * sizeof(int16_t)))
            {
                return false;
            }
        }
        else
        {
            std::copy(range.m_pcm->begin() + offset, range.m_pcm->begin() + offset + count, target);
        }
    }
    return true;
}