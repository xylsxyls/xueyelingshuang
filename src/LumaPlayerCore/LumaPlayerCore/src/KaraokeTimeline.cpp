#include "KaraokeTimeline.h"

#include <algorithm>
#include <limits>

KaraokeTimeline::KaraokeTimeline() :
m_mediaDuration100ns(0)
{

}

void KaraokeTimeline::swap(KaraokeTimeline& other)
{
    std::swap(m_mediaDuration100ns, other.m_mediaDuration100ns);
    m_capturedRanges.swap(other.m_capturedRanges);
}

void KaraokeTimeline::clear()
{
    m_mediaDuration100ns = 0;
    m_capturedRanges.clear();
}

bool KaraokeTimeline::advanceTo(int64_t mediaPosition100ns)
{
    if (mediaPosition100ns < 0)
    {
        return false;
    }
    if (mediaPosition100ns > m_mediaDuration100ns)
    {
        m_mediaDuration100ns = mediaPosition100ns;
    }
    return true;
}

bool KaraokeTimeline::replaceCapturedRange(int64_t mediaStart100ns,
    int64_t mediaEnd100ns, int32_t ratePermille)
{
    if (mediaStart100ns < 0 || mediaEnd100ns <= mediaStart100ns ||
        ratePermille < 100 || ratePermille > 5000)
    {
        return false;
    }

    std::vector<KaraokeTimelineCapturedRange> replacement;
    try
    {
        replacement.reserve(m_capturedRanges.size() + 2);
        for (size_t i = 0; i < m_capturedRanges.size(); ++i)
        {
            const KaraokeTimelineCapturedRange& current = m_capturedRanges[i];
            if (current.m_end100ns <= mediaStart100ns ||
                current.m_start100ns >= mediaEnd100ns)
            {
                replacement.push_back(current);
                continue;
            }
            if (current.m_start100ns < mediaStart100ns)
            {
                KaraokeTimelineCapturedRange left = current;
                left.m_end100ns = mediaStart100ns;
                replacement.push_back(left);
            }
            if (current.m_end100ns > mediaEnd100ns)
            {
                KaraokeTimelineCapturedRange right = current;
                right.m_start100ns = mediaEnd100ns;
                replacement.push_back(right);
            }
        }
        replacement.push_back(KaraokeTimelineCapturedRange());
        replacement.back().m_start100ns = mediaStart100ns;
        replacement.back().m_end100ns = mediaEnd100ns;
        replacement.back().m_ratePermille = ratePermille;
        /** 按原媒体起点排序新录制区间
        @param [in] left 借用的左侧候选区间
        @param [in] right 借用的右侧候选区间
        @return 左侧起点更早时返回true
        */
        std::sort(replacement.begin(), replacement.end(),
            [](const KaraokeTimelineCapturedRange& left, const KaraokeTimelineCapturedRange& right)
            {
                return left.m_start100ns < right.m_start100ns;
            });

        std::vector<KaraokeTimelineCapturedRange> merged;
        merged.reserve(replacement.size());
        for (size_t i = 0; i < replacement.size(); ++i)
        {
            if (!merged.empty() &&
                merged.back().m_end100ns == replacement[i].m_start100ns &&
                merged.back().m_ratePermille == replacement[i].m_ratePermille)
            {
                merged.back().m_end100ns = replacement[i].m_end100ns;
            }
            else
            {
                merged.push_back(replacement[i]);
            }
        }
        m_capturedRanges.swap(merged);
    }
    catch (...)
    {
        return false;
    }

    if (mediaEnd100ns > m_mediaDuration100ns)
    {
        m_mediaDuration100ns = mediaEnd100ns;
    }
    return true;
}

int64_t KaraokeTimeline::mediaDuration100ns() const
{
    return m_mediaDuration100ns;
}

bool KaraokeTimeline::scaleTime(int64_t value, int32_t numerator,
    int32_t denominator, int64_t* scaled)
{
    if (!scaled || value < 0 || numerator <= 0 || denominator <= 0)
    {
        return false;
    }
    const int64_t quotient = value / denominator;
    const int64_t remainder = value % denominator;
    const int64_t maxValue = (std::numeric_limits<int64_t>::max)();
    if (quotient > maxValue / numerator)
    {
        return false;
    }
    const int64_t whole = quotient * numerator;
    const int64_t fraction = (remainder * numerator + denominator / 2) / denominator;
    if (whole > maxValue - fraction)
    {
        return false;
    }
    *scaled = whole + fraction;
    if (value > 0 && *scaled == 0)
    {
        *scaled = 1;
    }
    return true;
}

std::vector<KaraokeTimelineSegment> KaraokeTimeline::segments() const
{
    std::vector<KaraokeTimelineSegment> result;
    int64_t mediaCursor = 0;
    int64_t outputCursor = 0;
    for (size_t i = 0; i <= m_capturedRanges.size(); ++i)
    {
        const bool hasRange = i < m_capturedRanges.size();
        const int64_t rangeStart = hasRange ? m_capturedRanges[i].m_start100ns : m_mediaDuration100ns;
        if (rangeStart > mediaCursor)
        {
            KaraokeTimelineSegment gap;
            gap.m_mediaStart100ns = mediaCursor;
            gap.m_mediaEnd100ns = rangeStart;
            gap.m_outputStart100ns = outputCursor;
            gap.m_ratePermille = 1000;
            gap.m_hasMicrophoneAudio = false;
            if (!scaleTime(rangeStart - mediaCursor, 1000, 1000, &gap.m_outputEnd100ns))
            {
                return std::vector<KaraokeTimelineSegment>();
            }
            if (gap.m_outputStart100ns >
                (std::numeric_limits<int64_t>::max)() - gap.m_outputEnd100ns)
            {
                return std::vector<KaraokeTimelineSegment>();
            }
            gap.m_outputEnd100ns += gap.m_outputStart100ns;
            outputCursor = gap.m_outputEnd100ns;
            result.push_back(gap);
            mediaCursor = rangeStart;
        }
        if (!hasRange)
        {
            break;
        }

        KaraokeTimelineSegment captured;
        captured.m_mediaStart100ns = m_capturedRanges[i].m_start100ns;
        captured.m_mediaEnd100ns = m_capturedRanges[i].m_end100ns;
        captured.m_outputStart100ns = outputCursor;
        captured.m_ratePermille = m_capturedRanges[i].m_ratePermille;
        captured.m_hasMicrophoneAudio = true;
        int64_t outputDuration = 0;
        if (!scaleTime(captured.m_mediaEnd100ns - captured.m_mediaStart100ns,
                1000, captured.m_ratePermille, &outputDuration) ||
            outputCursor > (std::numeric_limits<int64_t>::max)() - outputDuration)
        {
            return std::vector<KaraokeTimelineSegment>();
        }
        captured.m_outputEnd100ns = outputCursor + outputDuration;
        result.push_back(captured);
        outputCursor = captured.m_outputEnd100ns;
        mediaCursor = captured.m_mediaEnd100ns;
    }
    return result;
}

bool KaraokeTimeline::outputDuration100ns(int64_t* duration100ns) const
{
    if (!duration100ns)
    {
        return false;
    }
    const std::vector<KaraokeTimelineSegment> mapped = segments();
    if (m_mediaDuration100ns > 0 && mapped.empty())
    {
        return false;
    }
    *duration100ns = mapped.empty() ? 0 : mapped.back().m_outputEnd100ns;
    return true;
}

bool KaraokeTimeline::mediaToOutputTime100ns(int64_t mediaTime100ns,
    int64_t* outputTime100ns) const
{
    if (!outputTime100ns || mediaTime100ns < 0 || mediaTime100ns > m_mediaDuration100ns)
    {
        return false;
    }
    if (mediaTime100ns == 0)
    {
        *outputTime100ns = 0;
        return true;
    }
    const std::vector<KaraokeTimelineSegment> mapped = segments();
    if (mapped.empty())
    {
        return false;
    }
    for (size_t i = 0; i < mapped.size(); ++i)
    {
        const KaraokeTimelineSegment& segment = mapped[i];
        if (mediaTime100ns <= segment.m_mediaEnd100ns)
        {
            int64_t offset = 0;
            if (!scaleTime(mediaTime100ns - segment.m_mediaStart100ns,
                    1000, segment.m_ratePermille, &offset) ||
                segment.m_outputStart100ns > (std::numeric_limits<int64_t>::max)() - offset)
            {
                return false;
            }
            *outputTime100ns = segment.m_outputStart100ns + offset;
            if (*outputTime100ns > segment.m_outputEnd100ns)
            {
                *outputTime100ns = segment.m_outputEnd100ns;
            }
            return true;
        }
    }
    return false;
}

bool KaraokeTimeline::outputToMediaTime100ns(int64_t outputTime100ns,
    int64_t* mediaTime100ns) const
{
    int64_t totalOutput = 0;
    if (!mediaTime100ns || outputTime100ns < 0 || !outputDuration100ns(&totalOutput) ||
        outputTime100ns > totalOutput)
    {
        return false;
    }
    if (outputTime100ns == 0)
    {
        *mediaTime100ns = 0;
        return true;
    }
    const std::vector<KaraokeTimelineSegment> mapped = segments();
    for (size_t i = 0; i < mapped.size(); ++i)
    {
        const KaraokeTimelineSegment& segment = mapped[i];
        if (outputTime100ns <= segment.m_outputEnd100ns)
        {
            int64_t offset = 0;
            if (!scaleTime(outputTime100ns - segment.m_outputStart100ns,
                    segment.m_ratePermille, 1000, &offset) ||
                segment.m_mediaStart100ns > (std::numeric_limits<int64_t>::max)() - offset)
            {
                return false;
            }
            *mediaTime100ns = segment.m_mediaStart100ns + offset;
            if (*mediaTime100ns > segment.m_mediaEnd100ns)
            {
                *mediaTime100ns = segment.m_mediaEnd100ns;
            }
            return true;
        }
    }
    return false;
}