#include "FFmpegCppAdjacentReader.h"

// 一秒包含的100纳秒计时单位，AVRational分母使用32位整数
#define FFMPEG_ADJACENT_TICKS_PER_SECOND (1000 * 1000 * 10)
extern "C"
{
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavutil/imgutils.h>
#include <ffmpeg/libavutil/common.h>
}

FFmpegCppAdjacentReader::FFmpegCppAdjacentReader() :
m_maxFrames(16),
m_maxBytes(64 * 1024 * 1024),
m_cachedBytes(0),
m_draining(false),
m_ended(false)
{

}

FFmpegCppAdjacentReader::~FFmpegCppAdjacentReader()
{
    reset();
}

void FFmpegCppAdjacentReader::reset()
{
    for (size_t i = 0; i < m_frames.size(); ++i)
    {
        av_frame_free(&m_frames[i]);
    }
    m_frames.clear();
    m_cachedBytes = 0;
    m_draining = false;
    m_ended = false;
}

/** 估算帧的原始像素字节数，忽略行对齐及解码器内部引用
@param [in] frame 缓存持有的非空帧
@return 有效图像大小，未知格式返回0
*/
static int32_t AdjacentFrameBytes(const AVFrame* frame)
{
    const int32_t bytes = av_image_get_buffer_size(static_cast<AVPixelFormat>(frame->format), frame->width, frame->height, 1);
    return bytes > 0 ? bytes : 0;
}

bool FFmpegCppAdjacentReader::setLimits(int64_t maxFrames, int64_t maxBytes)
{
    if ((maxFrames != -1 && maxFrames < 3) || (maxBytes != -1 && maxBytes <= 0))
    {
        return false;
    }
    m_maxFrames = maxFrames;
    m_maxBytes = maxBytes;
    trim();
    return true;
}

void FFmpegCppAdjacentReader::trim()
{
    while (m_frames.size() > 3 && ((m_maxFrames >= 0 && m_frames.size() > static_cast<uint64_t>(m_maxFrames)) ||
        (m_maxBytes >= 0 && m_cachedBytes > m_maxBytes)))
    {
        AVFrame* first = m_frames.front();
        m_cachedBytes -= AdjacentFrameBytes(first);
        av_frame_free(&first);
        m_frames.pop_front();
    }
}

bool FFmpegCppAdjacentReader::canceled(const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackPreviewProfile* profile)
{
    bool stop = option.cancelCallback != nullptr && option.cancelCallback(option.cancelUserData);
    if (stop && profile != nullptr)
    {
        profile->canceled = true;
    }
    return stop;
}

int64_t FFmpegCppAdjacentReader::timestamp(const AVFrame* frame, AVFormatContext* format, int32_t streamIndex, int64_t start100ns)
{
    int64_t value = frame->best_effort_timestamp;
    if (value == AV_NOPTS_VALUE)
    {
        value = frame->pts;
    }
    if (value == AV_NOPTS_VALUE)
    {
        return -1;
    }
    AVRational target = { 1, FFMPEG_ADJACENT_TICKS_PER_SECOND };
    return av_sat_sub64(av_rescale_q(value, format->streams[streamIndex]->time_base, target), start100ns);
}

AVFrame* FFmpegCppAdjacentReader::decodeNext(AVFormatContext* format, AVCodecContext* codec, int32_t streamIndex,
    const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackPreviewProfile* profile)
{
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    if (frame == nullptr || packet == nullptr)
    {
        av_frame_free(&frame);
        av_packet_free(&packet);
        return nullptr;
    }
    while (!canceled(option, profile))
    {
        int32_t result = avcodec_receive_frame(codec, frame);
        if (result >= 0)
        {
            if (profile != nullptr)
            {
                ++profile->decodedVideoFrameCount;
            }
            av_packet_free(&packet);
            return frame;
        }
        if (result == AVERROR_EOF)
        {
            m_ended = true;
            break;
        }
        if (result != AVERROR(EAGAIN) || m_draining)
        {
            break;
        }
        // 接收端已排空后才送下一包，避免丢弃EAGAIN时未被接收的数据包
        do
        {
            av_packet_unref(packet);
            if (canceled(option, profile))
            {
                av_frame_free(&frame);
                av_packet_free(&packet);
                return nullptr;
            }
            result = av_read_frame(format, packet);
            if (profile != nullptr)
            {
                ++profile->readPacketCount;
            }
        } while (result >= 0 && packet->stream_index != streamIndex);
        if (result == AVERROR_EOF)
        {
            result = avcodec_send_packet(codec, nullptr);
            m_draining = true;
        }
        else if (result >= 0)
        {
            result = avcodec_send_packet(codec, packet);
        }
        av_packet_unref(packet);
        if (result < 0)
        {
            break;
        }
    }
    av_frame_free(&frame);
    av_packet_free(&packet);
    return nullptr;
}

const AVFrame* FFmpegCppAdjacentReader::read(AVFormatContext* format, AVCodecContext* codec, int32_t streamIndex,
    int64_t start100ns, int64_t duration100ns, int64_t origin100ns, int32_t direction,
    const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackVideoFrame* info,
    FFmpegCppPlaybackPreviewProfile* profile)
{
    if (format == nullptr || codec == nullptr || info == nullptr || direction == 0 || canceled(option, profile))
    {
        return nullptr;
    }
    if (direction < 0 && origin100ns <= 0)
    {
        return nullptr;
    }
    bool reuse = false;
    for (size_t i = 0; i < m_frames.size(); ++i)
    {
        if (timestamp(m_frames[i], format, streamIndex, start100ns) == origin100ns && (direction > 0 || i > 0))
        {
            reuse = true;
            break;
        }
    }
    int64_t seekPosition = origin100ns;
    if (!reuse)
    {
        reset();
        // 向左缺缓存时从更早位置顺序解码，不能用固定帧率反推相邻帧
        int64_t position = direction > 0 ? origin100ns : (origin100ns > FFMPEG_ADJACENT_TICKS_PER_SECOND ? origin100ns - FFMPEG_ADJACENT_TICKS_PER_SECOND : 0);
        seekPosition = position;
        AVRational source = { 1, FFMPEG_ADJACENT_TICKS_PER_SECOND };
        int64_t target = av_rescale_q_rnd(av_sat_add64(position, start100ns), source, format->streams[streamIndex]->time_base, AV_ROUND_DOWN);
        if (av_seek_frame(format, streamIndex, target, AVSEEK_FLAG_BACKWARD) < 0)
        {
            return nullptr;
        }
        avcodec_flush_buffers(codec);
    }
    const size_t frameBudget = option.frameBudget();
    size_t decodedFrames = 0;
    for (;;)
    {
        if (canceled(option, profile))
        {
            return nullptr;
        }
        size_t chosen = m_frames.size();
        for (size_t i = 0; i < m_frames.size(); ++i)
        {
            int64_t time = timestamp(m_frames[i], format, streamIndex, start100ns);
            if (direction > 0 && time > origin100ns)
            {
                chosen = i;
                break;
            }
            if (direction < 0 && time >= origin100ns)
            {
                if (i > 0)
                {
                    chosen = i - 1;
                }
                break;
            }
        }
        if (chosen < m_frames.size() && (chosen + 1 < m_frames.size() || m_ended))
        {
            info->timestamp100ns = timestamp(m_frames[chosen], format, streamIndex, start100ns);
            int64_t end = chosen + 1 < m_frames.size() ? timestamp(m_frames[chosen + 1], format, streamIndex, start100ns) : duration100ns;
            if (info->timestamp100ns < 0 || end <= info->timestamp100ns)
            {
                return nullptr;
            }
            info->duration100ns = end - info->timestamp100ns;
            return m_frames[chosen];
        }
        if (direction < 0 && chosen == m_frames.size() && !m_frames.empty() &&
            timestamp(m_frames.front(), format, streamIndex, start100ns) >= origin100ns)
        {
            if (seekPosition <= 0)
            {
                return nullptr;
            }
            int64_t distance = origin100ns - seekPosition;
            distance = distance > 0 ? av_sat_add64(distance, distance) : FFMPEG_ADJACENT_TICKS_PER_SECOND;
            seekPosition = origin100ns > distance ? origin100ns - distance : 0;
            reset();
            AVRational source = { 1, FFMPEG_ADJACENT_TICKS_PER_SECOND };
            int64_t target = av_rescale_q_rnd(av_sat_add64(seekPosition, start100ns), source, format->streams[streamIndex]->time_base, AV_ROUND_DOWN);
            if (av_seek_frame(format, streamIndex, target, AVSEEK_FLAG_BACKWARD) < 0)
            {
                return nullptr;
            }
            avcodec_flush_buffers(codec);
            continue;
        }
        if (m_ended)
        {
            return nullptr;
        }
        if (decodedFrames >= frameBudget)
        {
            return nullptr;
        }
        AVFrame* decoded = decodeNext(format, codec, streamIndex, option, profile);
        if (decoded == nullptr)
        {
            if (m_ended)
            {
                continue;
            }
            return nullptr;
        }
        ++decodedFrames;
        if (timestamp(decoded, format, streamIndex, start100ns) < 0)
        {
            av_frame_free(&decoded);
            return nullptr;
        }
        try
        {
            m_frames.push_back(decoded);
        }
        catch (...)
        {
            av_frame_free(&decoded);
            throw;
        }
        m_cachedBytes += AdjacentFrameBytes(decoded);
        trim();
    }
}