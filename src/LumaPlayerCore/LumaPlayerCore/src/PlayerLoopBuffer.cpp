#include "PlayerLoopBuffer.h"
#include "PlayerEngineHelper.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include <new>
#include <utility>

PlayerLoopBuffer::PlayerLoopBuffer(const LumaPlayerMediaInfo& media, int64_t start100ns, int64_t end100ns, int64_t maxBytes) :
CTask(kTaskId),
m_media(media),
m_start100ns(start100ns),
m_end100ns(end100ns),
m_maxBytes(maxBytes),
m_cachedBytes(0),
m_exit(false),
m_ready(false),
m_result(LumaPlayerCoreResultInternalError),
m_videoComplete(!media.m_hasVideo),
m_audioComplete(!media.m_hasAudio),
m_videoReader(nullptr),
m_audioReader(nullptr)
{

}

PlayerLoopBuffer::~PlayerLoopBuffer()
{
    delete m_videoReader;
    delete m_audioReader;
}

void PlayerLoopBuffer::DoTask()
{
	if (m_exit.load())
	{
		m_result = LumaPlayerCoreResultCanceled;
		m_ready.store(true);
		return;
	}
    try
    {
        m_result = prepareVideo();
        if (m_result == LumaPlayerCoreResultSuccess)
        {
            m_result = prepareAudio();
        }
    }
    catch (...)
    {
        m_result = LumaPlayerCoreResultInternalError;
    }
    if (m_exit.load())
    {
        m_result = LumaPlayerCoreResultCanceled;
    }
    m_ready.store(true);
}

void PlayerLoopBuffer::StopTask()
{
    m_exit.store(true);
}

bool PlayerLoopBuffer::matches(const LumaPlayerMediaInfo& media, int64_t start100ns, int64_t end100ns) const
{
    return m_media.m_filePath == media.m_filePath && m_start100ns == start100ns && m_end100ns == end100ns;
}

bool PlayerLoopBuffer::isReady() const
{
    return m_ready.load();
}

LumaPlayerCoreResult PlayerLoopBuffer::result() const
{
    return m_result;
}

bool PlayerLoopBuffer::isComplete() const
{
    return m_videoComplete && m_audioComplete;
}

LumaPlayerCoreResult PlayerLoopBuffer::videoFrame(size_t index, LumaPlayerVideoFrame* frame) const
{
    if (frame == nullptr)
    {
        return LumaPlayerCoreResultInvalidParam;
    }
    if (index < m_videoFrames.size())
    {
        *frame = m_videoFrames[index];
        return LumaPlayerCoreResultSuccess;
    }
    return m_videoComplete ? LumaPlayerCoreResultEnd : LumaPlayerCoreResultNotOpen;
}

LumaPlayerCoreResult PlayerLoopBuffer::audioFrame(size_t index, LumaPlayerAudioFrame* frame) const
{
    if (frame == nullptr)
    {
        return LumaPlayerCoreResultInvalidParam;
    }
    if (index < m_audioFrames.size())
    {
        *frame = m_audioFrames[index];
        return LumaPlayerCoreResultSuccess;
    }
    return m_audioComplete ? LumaPlayerCoreResultEnd : LumaPlayerCoreResultNotOpen;
}

FFmpegCppPlaybackReader* PlayerLoopBuffer::takeVideoReader()
{
    FFmpegCppPlaybackReader* reader = m_videoReader;
    m_videoReader = nullptr;
    return reader;
}

FFmpegCppPlaybackReader* PlayerLoopBuffer::takeAudioReader()
{
    FFmpegCppPlaybackReader* reader = m_audioReader;
    m_audioReader = nullptr;
    return reader;
}

LumaPlayerCoreResult PlayerLoopBuffer::prepareVideo()
{
    if (m_exit.load())
    {
        return LumaPlayerCoreResultCanceled;
    }
    if (!m_media.m_hasVideo)
    {
        return LumaPlayerCoreResultSuccess;
    }
    m_videoReader = new FFmpegCppPlaybackReader();
    FFmpegCppPlaybackOpenOption options;
    options.decodeVideo = true;
    options.decodeAudio = false;
    options.fallbackVideoFrameDuration100ns = m_media.m_defaultVideoFrameDuration100ns;
    if (!m_videoReader->open(m_media.m_filePath, options))
    {
        return LumaPlayerCoreResultVideoOpenFailed;
    }
    if (m_exit.load())
    {
        return LumaPlayerCoreResultCanceled;
    }
    if (!m_videoReader->seek(m_start100ns))
    {
        return LumaPlayerCoreResultVideoSeekFailed;
    }
    while (!m_exit.load())
    {
        FFmpegCppPlaybackFrame decoded;
        FFmpegCppPlaybackReadResult result = m_videoReader->read(&decoded);
        if (m_exit.load())
        {
            return LumaPlayerCoreResultCanceled;
        }
        if (result == FFmpegCppPlaybackReadResultEnd)
        {
            m_videoComplete = true;
            break;
        }
        if (result == FFmpegCppPlaybackReadResultError)
        {
            return LumaPlayerCoreResultVideoReadFailed;
        }
        if (decoded.type != FFmpegCppPlaybackFrameTypeVideo)
        {
            continue;
        }
        LumaPlayerVideoFrame frame;
        PlayerEngineHelper::convertVideoFrame(&decoded.videoFrame, -1, &frame);
        if (frame.endTime100ns() <= m_start100ns)
        {
            continue;
        }
        m_cachedBytes += static_cast<int64_t>(frame.m_bgraPixels.size());
        m_videoComplete = frame.endTime100ns() >= m_end100ns;
        m_videoFrames.push_back(std::move(frame));
        if (m_videoComplete || m_cachedBytes >= m_maxBytes * 3 / 4)
        {
            break;
        }
    }
    return m_exit.load() ? LumaPlayerCoreResultCanceled : LumaPlayerCoreResultSuccess;
}

LumaPlayerCoreResult PlayerLoopBuffer::prepareAudio()
{
    if (m_exit.load())
    {
        return LumaPlayerCoreResultCanceled;
    }
    if (!m_media.m_hasAudio)
    {
        return LumaPlayerCoreResultSuccess;
    }
    m_audioReader = new FFmpegCppPlaybackReader();
    FFmpegCppPlaybackOpenOption options;
    options.decodeVideo = false;
    options.decodeAudio = true;
    options.outputChannels = 2;
    options.outputBitsPerSample = 16;
    if (!m_audioReader->open(m_media.m_filePath, options))
    {
        return LumaPlayerCoreResultAudioOpenFailed;
    }
    if (m_exit.load())
    {
        return LumaPlayerCoreResultCanceled;
    }
    if (!m_audioReader->seek(m_start100ns))
    {
        return LumaPlayerCoreResultAudioSeekFailed;
    }
    while (!m_exit.load())
    {
        FFmpegCppPlaybackFrame decoded;
        FFmpegCppPlaybackReadResult result = m_audioReader->read(&decoded);
        if (m_exit.load())
        {
            return LumaPlayerCoreResultCanceled;
        }
        if (result == FFmpegCppPlaybackReadResultEnd)
        {
            m_audioComplete = true;
            break;
        }
        if (result == FFmpegCppPlaybackReadResultError)
        {
            return LumaPlayerCoreResultAudioReadFailed;
        }
        if (decoded.type != FFmpegCppPlaybackFrameTypeAudio)
        {
            continue;
        }
        LumaPlayerAudioFrame frame;
        PlayerEngineHelper::convertAudioFrame(&decoded.audioFrame, &frame);
        if (frame.endTime100ns() <= m_start100ns)
        {
            continue;
        }
        m_cachedBytes += static_cast<int64_t>(frame.m_pcmData.size());
        m_audioComplete = frame.endTime100ns() >= m_end100ns;
        m_audioFrames.push_back(std::move(frame));
        if (m_audioComplete || m_cachedBytes >= m_maxBytes)
        {
            break;
        }
    }
    return m_exit.load() ? LumaPlayerCoreResultCanceled : LumaPlayerCoreResultSuccess;
}