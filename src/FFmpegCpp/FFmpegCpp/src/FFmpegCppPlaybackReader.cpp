#include "FFmpegCppPlaybackReader.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <exception>
#include <new>
#include <sstream>

#ifdef _MSC_VER
#pragma warning(disable:4819)
#pragma warning(disable:4996)
#endif

#ifdef _WIN32
#ifdef _DEBUG
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avutil.lib")
#else
#ifdef _WIN64
#pragma comment(lib, "avformatstatic64.lib")
#pragma comment(lib, "avcodecstatic64.lib")
#pragma comment(lib, "swscalestatic64.lib")
#pragma comment(lib, "swresamplestatic64.lib")
#pragma comment(lib, "avutilstatic64.lib")
#else
#pragma comment(lib, "avformatstatic32.lib")
#pragma comment(lib, "avcodecstatic32.lib")
#pragma comment(lib, "swscalestatic32.lib")
#pragma comment(lib, "swresamplestatic32.lib")
#pragma comment(lib, "avutilstatic32.lib")
#endif
#endif
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "bcrypt.lib")
#endif

extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libavutil/channel_layout.h>
#include <ffmpeg/libavutil/error.h>
#include <ffmpeg/libavutil/imgutils.h>
#include <ffmpeg/libavutil/samplefmt.h>
#include <ffmpeg/libswresample/swresample.h>
#include <ffmpeg/libswscale/swscale.h>
}

namespace
{
    static const int64_t kPlaybackOneSecond100ns = 10000000LL;
    static const int64_t kPlaybackDefaultFrameDuration100ns = kPlaybackOneSecond100ns / 30;
    static const size_t kPlaybackDefaultPreviewFrameCount = 120;

    /** 把FFmpeg错误码转换为可读文本
    @param [in] errorCode FFmpeg错误码
    @return 返回错误文本
    */
    std::string playbackErrorText(int errorCode)
    {
        char buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        if (av_strerror(errorCode, buffer, sizeof(buffer)) == 0)
        {
            return buffer;
        }

        std::ostringstream stream;
        stream << "ffmpeg error " << errorCode;
        return stream.str();
    }

    /** 拼接带FFmpeg错误码的错误文本
    @param [in] prefix 错误前缀
    @param [in] errorCode FFmpeg错误码
    @return 返回错误文本
    */
    std::string playbackAppendError(const char* prefix, int errorCode)
    {
        std::ostringstream stream;
        stream << (prefix ? prefix : "ffmpeg failed") << ": " << playbackErrorText(errorCode);
        return stream.str();
    }

    /** 把FFmpeg时间戳换算成100ns单位
    @param [in] timestamp FFmpeg时间戳
    @param [in] timeBase 时间基
    @return 返回100ns时间戳，未知时返回-1
    */
    int64_t playbackRescaleTo100ns(int64_t timestamp, AVRational timeBase)
    {
        if (timestamp == AV_NOPTS_VALUE || timeBase.num == 0 || timeBase.den == 0)
        {
            return -1;
        }

        AVRational targetBase;
        targetBase.num = 1;
        targetBase.den = static_cast<int>(kPlaybackOneSecond100ns);
        return av_rescale_q(timestamp, timeBase, targetBase);
    }

    /** 把100ns时间戳换算成FFmpeg时间戳
    @param [in] timestamp100ns 100ns时间戳
    @param [in] timeBase 目标时间基
    @return 返回FFmpeg时间戳
    */
    int64_t playbackRescaleFrom100ns(int64_t timestamp100ns, AVRational timeBase)
    {
        AVRational sourceBase;
        sourceBase.num = 1;
        sourceBase.den = static_cast<int>(kPlaybackOneSecond100ns);
        return av_rescale_q(timestamp100ns, sourceBase, timeBase);
    }

    /** 把微秒时间戳换算成100ns
    @param [in] timestampUs 微秒时间戳
    @return 返回100ns时间戳
    */
    int64_t playbackMicrosecondsTo100ns(int64_t timestampUs)
    {
        if (timestampUs == AV_NOPTS_VALUE)
        {
            return -1;
        }
        return timestampUs * 10;
    }

    /** 把FFmpeg媒体类型转换为公开枚举
    @param [in] mediaType FFmpeg媒体类型
    @return 返回公开媒体类型
    */
    FFmpegCppMediaType playbackConvertMediaType(AVMediaType mediaType)
    {
        if (mediaType == AVMEDIA_TYPE_VIDEO)
        {
            return FFmpegCppMediaTypeVideo;
        }
        if (mediaType == AVMEDIA_TYPE_AUDIO)
        {
            return FFmpegCppMediaTypeAudio;
        }
        if (mediaType == AVMEDIA_TYPE_SUBTITLE)
        {
            return FFmpegCppMediaTypeSubtitle;
        }
        return FFmpegCppMediaTypeUnknown;
    }

    /** 把FFmpeg有理数转换为公开结构
    @param [in] rational FFmpeg有理数
    @return 返回公开有理数
    */
    FFmpegCppRational playbackConvertRational(AVRational rational)
    {
        return FFmpegCppRational(rational.num, rational.den);
    }

    /** 读取帧的最佳显示时间戳
    @param [in] frame FFmpeg帧
    @return 返回时间戳，未知时返回AV_NOPTS_VALUE
    */
    int64_t playbackBestFrameTimestamp(const AVFrame* frame)
    {
        if (frame == nullptr)
        {
            return AV_NOPTS_VALUE;
        }
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE)
        {
            return frame->best_effort_timestamp;
        }
        if (frame->pts != AV_NOPTS_VALUE)
        {
            return frame->pts;
        }
        if (frame->pkt_dts != AV_NOPTS_VALUE)
        {
            return frame->pkt_dts;
        }
        return AV_NOPTS_VALUE;
    }

    /** 裁剪播放位置到媒体范围内
    @param [in] value 输入时间
    @param [in] duration 媒体总时长
    @return 返回裁剪后的时间
    */
    int64_t playbackClampPosition(int64_t value, int64_t duration)
    {
        if (value < 0)
        {
            value = 0;
        }
        if (duration > 0 && value > duration)
        {
            value = duration;
        }
        return value;
    }

    /** 根据流和容器计算归一化起点
    @param [in] formatContext 容器上下文
    @param [in] stream 流
    @return 返回起点，单位100ns
    */
    int64_t playbackStreamStart100ns(const AVFormatContext* formatContext, const AVStream* stream)
    {
        if (stream != nullptr && stream->start_time != AV_NOPTS_VALUE)
        {
            int64_t streamStart = playbackRescaleTo100ns(stream->start_time, stream->time_base);
            if (streamStart >= 0)
            {
                return streamStart;
            }
        }

        if (formatContext != nullptr && formatContext->start_time != AV_NOPTS_VALUE)
        {
            int64_t formatStart = playbackMicrosecondsTo100ns(formatContext->start_time);
            if (formatStart >= 0)
            {
                return formatStart;
            }
        }

        return 0;
    }

    /** 根据流帧率估算视频帧时长
    @param [in] formatContext 容器上下文
    @param [in] stream 视频流
    @param [in] fallback100ns 兜底帧时长
    @return 返回估算帧时长，单位100ns
    */
    int64_t playbackGuessFrameDuration100ns(AVFormatContext* formatContext, AVStream* stream, int64_t fallback100ns)
    {
        if (fallback100ns <= 0)
        {
            fallback100ns = kPlaybackDefaultFrameDuration100ns;
        }

        if (stream == nullptr)
        {
            return fallback100ns;
        }

        AVRational frameRate = av_guess_frame_rate(formatContext, stream, nullptr);
        if (frameRate.num == 0 || frameRate.den == 0)
        {
            frameRate = stream->avg_frame_rate.num != 0 && stream->avg_frame_rate.den != 0 ? stream->avg_frame_rate : stream->r_frame_rate;
        }
        if (frameRate.num > 0 && frameRate.den > 0)
        {
            AVRational targetBase;
            targetBase.num = 1;
            targetBase.den = static_cast<int>(kPlaybackOneSecond100ns);
            return av_rescale_q(1, av_inv_q(frameRate), targetBase);
        }

        return fallback100ns;
    }

    /** 构造公开流信息
    @param [in] formatContext 容器上下文
    @param [in] stream FFmpeg流
    @param [in] mainVideoStreamIndex 主视频流下标
    @param [in] mainAudioStreamIndex 主音频流下标
    @return 返回公开流信息
    */
    FFmpegCppStreamInfo playbackMakeStreamInfo(const AVFormatContext* formatContext,
                                               const AVStream* stream,
                                               int32_t mainVideoStreamIndex,
                                               int32_t mainAudioStreamIndex)
    {
        FFmpegCppStreamInfo streamInfo;
        if (formatContext == nullptr || stream == nullptr || stream->codecpar == nullptr)
        {
            return streamInfo;
        }

        const AVCodecParameters* codecParameters = stream->codecpar;
        const AVCodecDescriptor* codecDescriptor = avcodec_descriptor_get(codecParameters->codec_id);
        const char* codecName = avcodec_get_name(codecParameters->codec_id);
        const char* profileName = avcodec_profile_name(codecParameters->codec_id, codecParameters->profile);

        streamInfo.streamIndex = static_cast<int32_t>(stream->index);
        streamInfo.mediaType = playbackConvertMediaType(codecParameters->codec_type);
        streamInfo.codecName = codecName != nullptr ? codecName : std::string();
        streamInfo.codecLongName = codecDescriptor != nullptr && codecDescriptor->long_name != nullptr ? codecDescriptor->long_name : std::string();
        streamInfo.profile = profileName != nullptr ? profileName : std::string();
        streamInfo.width = codecParameters->width;
        streamInfo.height = codecParameters->height;
        streamInfo.bitRate = codecParameters->bit_rate;
        streamInfo.timeBase = playbackConvertRational(stream->time_base);
        streamInfo.sampleRate = codecParameters->sample_rate;
        streamInfo.channels = codecParameters->ch_layout.nb_channels;

        AVRational frameRate = stream->avg_frame_rate.num != 0 && stream->avg_frame_rate.den != 0 ? stream->avg_frame_rate : stream->r_frame_rate;
        streamInfo.frameRate = playbackConvertRational(frameRate);
        if (stream->duration != AV_NOPTS_VALUE)
        {
            int64_t duration100ns = playbackRescaleTo100ns(stream->duration, stream->time_base);
            streamInfo.durationMilliseconds = duration100ns > 0 ? duration100ns / 10000 : 0;
        }
        else if (formatContext->duration != AV_NOPTS_VALUE)
        {
            streamInfo.durationMilliseconds = formatContext->duration * 1000 / AV_TIME_BASE;
        }

        if (streamInfo.streamIndex == mainVideoStreamIndex)
        {
            streamInfo.mediaType = FFmpegCppMediaTypeVideo;
        }
        if (streamInfo.streamIndex == mainAudioStreamIndex)
        {
            streamInfo.mediaType = FFmpegCppMediaTypeAudio;
        }
        return streamInfo;
    }

    /** 读取音频声道数
    @param [in] codecContext 音频解码上下文
    @return 返回声道数
    */
    int playbackAudioChannelCount(const AVCodecContext* codecContext)
    {
        if (codecContext == nullptr)
        {
            return 0;
        }
        if (codecContext->ch_layout.nb_channels > 0)
        {
            return codecContext->ch_layout.nb_channels;
        }
        return codecContext->channels;
    }

    /** 初始化音频声道布局
    @param [in] codecContext 音频解码上下文
    @param [out] layout 输出声道布局
    */
    void playbackInitInputChannelLayout(const AVCodecContext* codecContext, AVChannelLayout* layout)
    {
        if (layout == nullptr)
        {
            return;
        }

        memset(layout, 0, sizeof(*layout));
        if (codecContext != nullptr && av_channel_layout_check(&codecContext->ch_layout))
        {
            av_channel_layout_copy(layout, &codecContext->ch_layout);
            return;
        }

        int channels = playbackAudioChannelCount(codecContext);
        if (channels <= 0)
        {
            channels = 2;
        }
        av_channel_layout_default(layout, channels);
    }
}

/** FFmpegCppPlaybackReader私有实现
*/
struct FFmpegCppPlaybackReaderImpl
{
public:
    FFmpegCppPlaybackReaderImpl()
        : m_formatContext(nullptr),
          m_videoCodecContext(nullptr),
          m_audioCodecContext(nullptr),
          m_scaleContext(nullptr),
          m_resampleContext(nullptr),
          m_packet(nullptr),
          m_frame(nullptr),
          m_videoStreamIndex(-1),
          m_audioStreamIndex(-1),
          m_videoStartTime100ns(0),
          m_audioStartTime100ns(0),
          m_duration100ns(0),
          m_videoFrameDuration100ns(kPlaybackDefaultFrameDuration100ns),
          m_nextVideoTimestamp100ns(0),
          m_nextAudioTimestamp100ns(0),
          m_videoWidth(0),
          m_videoHeight(0),
          m_audioSampleRate(0),
          m_audioChannels(0),
          m_audioBitsPerSample(16),
          m_isOpen(false),
          m_drainStarted(false),
          m_videoDrained(false),
          m_audioDrained(false),
          m_readPacketCount(0),
          m_videoFrameCount(0),
          m_audioFrameCount(0)
    {
        m_videoTimeBase.num = 1;
        m_videoTimeBase.den = static_cast<int>(kPlaybackOneSecond100ns);
        m_audioTimeBase = m_videoTimeBase;
        av_log_set_level(AV_LOG_ERROR);
    }

    ~FFmpegCppPlaybackReaderImpl()
    {
        close();
    }

    bool open(const std::string& filePath, const FFmpegCppPlaybackOpenOption& option)
    {
        close();
        m_lastError.clear();
        m_filePath = filePath;
        m_option = option;
        if (m_filePath.empty())
        {
            m_lastError = "filePath is empty";
            return false;
        }

        int result = avformat_open_input(&m_formatContext, m_filePath.c_str(), nullptr, nullptr);
        if (result < 0)
        {
            m_lastError = playbackAppendError("avformat_open_input failed", result);
            close();
            return false;
        }

        result = avformat_find_stream_info(m_formatContext, nullptr);
        if (result < 0)
        {
            m_lastError = playbackAppendError("avformat_find_stream_info failed", result);
            close();
            return false;
        }

        m_videoStreamIndex = option.decodeVideo ? av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0) : -1;
        m_audioStreamIndex = option.decodeAudio ? av_find_best_stream(m_formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) : -1;

        if (m_videoStreamIndex >= 0 && !openDecoder(m_videoStreamIndex, true))
        {
            m_videoStreamIndex = -1;
        }
        if (m_audioStreamIndex >= 0 && !openDecoder(m_audioStreamIndex, false))
        {
            m_audioStreamIndex = -1;
        }

        if (m_videoStreamIndex < 0 && m_audioStreamIndex < 0)
        {
            if (m_lastError.empty())
            {
                m_lastError = "no playable audio or video stream";
            }
            close();
            return false;
        }

        m_packet = av_packet_alloc();
        m_frame = av_frame_alloc();
        if (m_packet == nullptr || m_frame == nullptr)
        {
            m_lastError = "alloc packet/frame failed";
            close();
            return false;
        }

        fillMediaInfo();
        resetReadState(0);
        m_isOpen = true;
        m_lastError.clear();
        return true;
    }

    void close()
    {
        m_frameQueue.clear();
        if (m_packet != nullptr)
        {
            av_packet_free(&m_packet);
            m_packet = nullptr;
        }
        if (m_frame != nullptr)
        {
            av_frame_free(&m_frame);
            m_frame = nullptr;
        }
        if (m_scaleContext != nullptr)
        {
            sws_freeContext(m_scaleContext);
            m_scaleContext = nullptr;
        }
        if (m_resampleContext != nullptr)
        {
            swr_free(&m_resampleContext);
            m_resampleContext = nullptr;
        }
        if (m_videoCodecContext != nullptr)
        {
            avcodec_free_context(&m_videoCodecContext);
            m_videoCodecContext = nullptr;
        }
        if (m_audioCodecContext != nullptr)
        {
            avcodec_free_context(&m_audioCodecContext);
            m_audioCodecContext = nullptr;
        }
        if (m_formatContext != nullptr)
        {
            avformat_close_input(&m_formatContext);
            m_formatContext = nullptr;
        }

        m_mediaInfo.reset();
        m_filePath.clear();
        m_isOpen = false;
        m_videoStreamIndex = -1;
        m_audioStreamIndex = -1;
        m_videoStartTime100ns = 0;
        m_audioStartTime100ns = 0;
        m_duration100ns = 0;
        m_videoFrameDuration100ns = kPlaybackDefaultFrameDuration100ns;
        m_nextVideoTimestamp100ns = 0;
        m_nextAudioTimestamp100ns = 0;
        m_videoWidth = 0;
        m_videoHeight = 0;
        m_audioSampleRate = 0;
        m_audioChannels = 0;
        m_audioBitsPerSample = 16;
        m_drainStarted = false;
        m_videoDrained = false;
        m_audioDrained = false;
        m_readPacketCount = 0;
        m_videoFrameCount = 0;
        m_audioFrameCount = 0;
    }

    bool isOpen() const
    {
        return m_isOpen && m_formatContext != nullptr;
    }

    bool getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const
    {
        if (mediaInfo == nullptr || !isOpen())
        {
            return false;
        }
        *mediaInfo = m_mediaInfo;
        return true;
    }

    bool getVideoSize(int32_t* width, int32_t* height) const
    {
        if (width != nullptr)
        {
            *width = m_videoWidth;
        }
        if (height != nullptr)
        {
            *height = m_videoHeight;
        }
        return m_videoWidth > 0 && m_videoHeight > 0;
    }

    bool getAudioFormat(int32_t* sampleRate, int32_t* channels, int32_t* bitsPerSample) const
    {
        if (sampleRate != nullptr)
        {
            *sampleRate = m_audioSampleRate;
        }
        if (channels != nullptr)
        {
            *channels = m_audioChannels;
        }
        if (bitsPerSample != nullptr)
        {
            *bitsPerSample = m_audioBitsPerSample;
        }
        return m_audioCodecContext != nullptr && m_audioSampleRate > 0 && m_audioChannels > 0;
    }

    bool seek(int64_t position100ns)
    {
        if (!isOpen())
        {
            m_lastError = "reader is not open";
            return false;
        }

        position100ns = playbackClampPosition(position100ns, m_duration100ns);
        int targetStreamIndex = m_videoStreamIndex >= 0 ? m_videoStreamIndex : m_audioStreamIndex;
        int result = 0;
        if (targetStreamIndex >= 0 && m_formatContext->streams[targetStreamIndex] != nullptr)
        {
            AVStream* stream = m_formatContext->streams[targetStreamIndex];
            const int64_t start100ns = targetStreamIndex == m_videoStreamIndex ? m_videoStartTime100ns : m_audioStartTime100ns;
            const int64_t target = playbackRescaleFrom100ns(position100ns + start100ns, stream->time_base);
            result = av_seek_frame(m_formatContext, targetStreamIndex, target, AVSEEK_FLAG_BACKWARD);
        }

        if (result < 0)
        {
            int64_t targetUs = position100ns / 10;
            if (m_formatContext->start_time != AV_NOPTS_VALUE)
            {
                targetUs += m_formatContext->start_time;
            }
            result = av_seek_frame(m_formatContext, -1, targetUs, AVSEEK_FLAG_BACKWARD);
        }

        if (result < 0)
        {
            m_lastError = playbackAppendError("av_seek_frame failed", result);
            return false;
        }

        if (m_videoCodecContext != nullptr)
        {
            avcodec_flush_buffers(m_videoCodecContext);
        }
        if (m_audioCodecContext != nullptr)
        {
            avcodec_flush_buffers(m_audioCodecContext);
        }
        resetReadState(position100ns);
        return true;
    }

    FFmpegCppPlaybackReadResult read(FFmpegCppPlaybackFrame* frame)
    {
        if (frame == nullptr)
        {
            m_lastError = "frame is null";
            return FFmpegCppPlaybackReadResultError;
        }
        frame->reset();
        if (!isOpen())
        {
            m_lastError = "reader is not open";
            return FFmpegCppPlaybackReadResultError;
        }

        for (;;)
        {
            if (popQueuedFrame(frame))
            {
                return FFmpegCppPlaybackReadResultFrame;
            }

            if (m_drainStarted)
            {
                if (!drainOneDecoder())
                {
                    return FFmpegCppPlaybackReadResultError;
                }
                if (popQueuedFrame(frame))
                {
                    return FFmpegCppPlaybackReadResultFrame;
                }
                if ((m_videoCodecContext == nullptr || m_videoDrained) &&
                    (m_audioCodecContext == nullptr || m_audioDrained))
                {
                    return FFmpegCppPlaybackReadResultEnd;
                }
                continue;
            }

            int result = av_read_frame(m_formatContext, m_packet);
            if (result == AVERROR_EOF)
            {
                m_drainStarted = true;
                continue;
            }
            if (result < 0)
            {
                m_lastError = playbackAppendError("av_read_frame failed", result);
                av_packet_unref(m_packet);
                return FFmpegCppPlaybackReadResultError;
            }

            ++m_readPacketCount;
            bool decodeOk = true;
            if (m_packet->stream_index == m_videoStreamIndex && m_videoCodecContext != nullptr)
            {
                decodeOk = decodePacket(m_videoCodecContext, m_packet, true);
            }
            else if (m_packet->stream_index == m_audioStreamIndex && m_audioCodecContext != nullptr)
            {
                decodeOk = decodePacket(m_audioCodecContext, m_packet, false);
            }
            av_packet_unref(m_packet);
            if (!decodeOk)
            {
                return FFmpegCppPlaybackReadResultError;
            }
        }
    }

    bool readVideoFrameAt(int64_t position100ns, size_t maxReadFrameCount, FFmpegCppPlaybackVideoFrame* frame)
    {
        if (frame == nullptr)
        {
            m_lastError = "video frame is null";
            return false;
        }
        frame->reset();
        if (!isOpen())
        {
            m_lastError = "reader is not open";
            return false;
        }
        if (m_videoCodecContext == nullptr)
        {
            m_lastError = "video stream is not open";
            return false;
        }

        position100ns = playbackClampPosition(position100ns, m_duration100ns);
        if (maxReadFrameCount == 0)
        {
            maxReadFrameCount = kPlaybackDefaultPreviewFrameCount;
        }

        if (!seek(position100ns))
        {
            return false;
        }

        bool haveFrame = false;
        FFmpegCppPlaybackVideoFrame bestFrame;
        const int64_t tolerance = std::max<int64_t>(1, m_videoFrameDuration100ns / 3);
        size_t readVideoCount = 0;

        while (readVideoCount < maxReadFrameCount)
        {
            FFmpegCppPlaybackFrame decodedFrame;
            FFmpegCppPlaybackReadResult result = read(&decodedFrame);
            if (result == FFmpegCppPlaybackReadResultEnd)
            {
                break;
            }
            if (result == FFmpegCppPlaybackReadResultError)
            {
                return false;
            }
            if (decodedFrame.type != FFmpegCppPlaybackFrameTypeVideo)
            {
                continue;
            }

            ++readVideoCount;
            const int64_t frameTime = decodedFrame.videoFrame.timestamp100ns;
            if (!haveFrame || frameTime <= position100ns + tolerance)
            {
                bestFrame = decodedFrame.videoFrame;
                haveFrame = true;
            }
            if (frameTime > position100ns + tolerance && haveFrame)
            {
                break;
            }
        }

        if (!haveFrame)
        {
            m_lastError = "video preview frame is not found";
            return false;
        }

        *frame = bestFrame;
        return true;
    }

private:
    bool openDecoder(int streamIndex, bool video)
    {
        if (m_formatContext == nullptr || streamIndex < 0 || static_cast<unsigned int>(streamIndex) >= m_formatContext->nb_streams)
        {
            m_lastError = "invalid stream index";
            return false;
        }

        AVStream* stream = m_formatContext->streams[streamIndex];
        if (stream == nullptr || stream->codecpar == nullptr)
        {
            m_lastError = "invalid stream";
            return false;
        }

        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (codec == nullptr)
        {
            std::ostringstream text;
            text << "decoder is not found for stream " << streamIndex;
            m_lastError = text.str();
            return false;
        }

        AVCodecContext* codecContext = avcodec_alloc_context3(codec);
        if (codecContext == nullptr)
        {
            m_lastError = "avcodec_alloc_context3 failed";
            return false;
        }

        int result = avcodec_parameters_to_context(codecContext, stream->codecpar);
        if (result < 0)
        {
            m_lastError = playbackAppendError("avcodec_parameters_to_context failed", result);
            avcodec_free_context(&codecContext);
            return false;
        }

        codecContext->pkt_timebase = stream->time_base;
        codecContext->thread_count = 0;
        result = avcodec_open2(codecContext, codec, nullptr);
        if (result < 0)
        {
            m_lastError = playbackAppendError("avcodec_open2 failed", result);
            avcodec_free_context(&codecContext);
            return false;
        }

        if (video)
        {
            m_videoCodecContext = codecContext;
            m_videoTimeBase = stream->time_base;
            m_videoStartTime100ns = playbackStreamStart100ns(m_formatContext, stream);
            m_videoFrameDuration100ns = playbackGuessFrameDuration100ns(m_formatContext,
                                                                         stream,
                                                                         m_option.fallbackVideoFrameDuration100ns);
            m_videoWidth = codecContext->width;
            m_videoHeight = codecContext->height;
        }
        else
        {
            m_audioCodecContext = codecContext;
            m_audioTimeBase = stream->time_base;
            m_audioStartTime100ns = playbackStreamStart100ns(m_formatContext, stream);
            if (!openAudioResampler())
            {
                avcodec_free_context(&m_audioCodecContext);
                return false;
            }
        }
        return true;
    }

    bool openAudioResampler()
    {
        if (m_audioCodecContext == nullptr)
        {
            return false;
        }

        const int inputSampleRate = m_audioCodecContext->sample_rate > 0 ? m_audioCodecContext->sample_rate : 44100;
        int inputChannels = playbackAudioChannelCount(m_audioCodecContext);
        if (inputChannels <= 0)
        {
            inputChannels = 2;
        }

        m_audioSampleRate = m_option.outputSampleRate > 0 ? m_option.outputSampleRate : inputSampleRate;
        m_audioChannels = m_option.outputChannels > 0 ? m_option.outputChannels : std::min<int>(2, std::max<int>(1, inputChannels));
        m_audioBitsPerSample = 16;

        AVChannelLayout inputLayout;
        AVChannelLayout outputLayout;
        playbackInitInputChannelLayout(m_audioCodecContext, &inputLayout);
        av_channel_layout_default(&outputLayout, m_audioChannels);

        int result = swr_alloc_set_opts2(&m_resampleContext,
                                         &outputLayout,
                                         AV_SAMPLE_FMT_S16,
                                         m_audioSampleRate,
                                         &inputLayout,
                                         m_audioCodecContext->sample_fmt,
                                         inputSampleRate,
                                         0,
                                         nullptr);
        av_channel_layout_uninit(&inputLayout);
        av_channel_layout_uninit(&outputLayout);
        if (result < 0 || m_resampleContext == nullptr)
        {
            m_lastError = playbackAppendError("swr_alloc_set_opts2 failed", result);
            return false;
        }

        result = swr_init(m_resampleContext);
        if (result < 0)
        {
            m_lastError = playbackAppendError("swr_init failed", result);
            swr_free(&m_resampleContext);
            return false;
        }
        return true;
    }

    void fillMediaInfo()
    {
        m_mediaInfo.reset();
        if (m_formatContext == nullptr)
        {
            return;
        }

        m_mediaInfo.filePath = m_filePath;
        if (m_formatContext->iformat != nullptr)
        {
            m_mediaInfo.formatName = m_formatContext->iformat->name != nullptr ? m_formatContext->iformat->name : std::string();
            m_mediaInfo.formatLongName = m_formatContext->iformat->long_name != nullptr ? m_formatContext->iformat->long_name : std::string();
        }
        if (m_formatContext->duration != AV_NOPTS_VALUE)
        {
            m_duration100ns = playbackMicrosecondsTo100ns(m_formatContext->duration);
            m_mediaInfo.durationMilliseconds = m_duration100ns / 10000;
        }
        m_mediaInfo.bitRate = m_formatContext->bit_rate;
        m_mediaInfo.mainVideoStreamIndex = m_videoStreamIndex;
        m_mediaInfo.mainAudioStreamIndex = m_audioStreamIndex;
        m_mediaInfo.hasVideo = m_videoStreamIndex >= 0;
        m_mediaInfo.hasAudio = m_audioStreamIndex >= 0;

        for (unsigned int i = 0; i < m_formatContext->nb_streams; ++i)
        {
            m_mediaInfo.streams.push_back(playbackMakeStreamInfo(m_formatContext,
                                                                 m_formatContext->streams[i],
                                                                 m_videoStreamIndex,
                                                                 m_audioStreamIndex));
            if (m_formatContext->streams[i] != nullptr && m_formatContext->streams[i]->duration != AV_NOPTS_VALUE)
            {
                int64_t streamDuration = playbackRescaleTo100ns(m_formatContext->streams[i]->duration,
                                                                 m_formatContext->streams[i]->time_base);
                if (streamDuration > m_duration100ns)
                {
                    m_duration100ns = streamDuration;
                    m_mediaInfo.durationMilliseconds = streamDuration / 10000;
                }
            }
        }
        if (m_duration100ns <= 0)
        {
            m_duration100ns = m_videoFrameDuration100ns;
            m_mediaInfo.durationMilliseconds = m_duration100ns / 10000;
        }
    }

    void resetReadState(int64_t position100ns)
    {
        m_frameQueue.clear();
        m_drainStarted = false;
        m_videoDrained = false;
        m_audioDrained = false;
        m_nextVideoTimestamp100ns = playbackClampPosition(position100ns, m_duration100ns);
        m_nextAudioTimestamp100ns = m_nextVideoTimestamp100ns;
        m_readPacketCount = 0;
        m_videoFrameCount = 0;
        m_audioFrameCount = 0;
    }

    bool popQueuedFrame(FFmpegCppPlaybackFrame* frame)
    {
        if (frame == nullptr || m_frameQueue.empty())
        {
            return false;
        }
        *frame = m_frameQueue.front();
        m_frameQueue.pop_front();
        return true;
    }

    bool drainOneDecoder()
    {
        if (m_videoCodecContext != nullptr && !m_videoDrained)
        {
            if (!decodePacket(m_videoCodecContext, nullptr, true))
            {
                return false;
            }
            if (!m_frameQueue.empty())
            {
                return true;
            }
        }

        if (m_audioCodecContext != nullptr && !m_audioDrained)
        {
            if (!decodePacket(m_audioCodecContext, nullptr, false))
            {
                return false;
            }
        }
        return true;
    }

    bool decodePacket(AVCodecContext* codecContext, AVPacket* packet, bool video)
    {
        int result = avcodec_send_packet(codecContext, packet);
        if (result == AVERROR_EOF)
        {
            if (video)
            {
                m_videoDrained = true;
            }
            else
            {
                m_audioDrained = true;
            }
            return true;
        }
        if (result == AVERROR(EAGAIN))
        {
            if (!receiveFrames(codecContext, video))
            {
                return false;
            }
            result = avcodec_send_packet(codecContext, packet);
        }
        if (result < 0)
        {
            m_lastError = playbackAppendError(video ? "send video packet failed" : "send audio packet failed", result);
            return false;
        }
        return receiveFrames(codecContext, video);
    }

    bool receiveFrames(AVCodecContext* codecContext, bool video)
    {
        for (;;)
        {
            int result = avcodec_receive_frame(codecContext, m_frame);
            if (result == AVERROR(EAGAIN))
            {
                return true;
            }
            if (result == AVERROR_EOF)
            {
                if (video)
                {
                    m_videoDrained = true;
                }
                else
                {
                    m_audioDrained = true;
                }
                return true;
            }
            if (result < 0)
            {
                m_lastError = playbackAppendError(video ? "receive video frame failed" : "receive audio frame failed", result);
                return false;
            }

            bool converted = video ? queueVideoFrame(m_frame) : queueAudioFrame(m_frame);
            av_frame_unref(m_frame);
            if (!converted)
            {
                return false;
            }
        }
    }

    bool queueVideoFrame(const AVFrame* frame)
    {
        if (frame == nullptr || m_videoCodecContext == nullptr)
        {
            return true;
        }
        if (frame->width <= 0 || frame->height <= 0)
        {
            m_lastError = "invalid video frame size";
            return false;
        }

        FFmpegCppPlaybackFrame playbackFrame;
        playbackFrame.type = FFmpegCppPlaybackFrameTypeVideo;
        playbackFrame.videoFrame.width = frame->width;
        playbackFrame.videoFrame.height = frame->height;
        playbackFrame.videoFrame.stride = frame->width * 4;
        playbackFrame.videoFrame.keyFrame = frame->key_frame != 0;

        const int64_t rawTimestamp = playbackBestFrameTimestamp(frame);
        int64_t timestamp100ns = playbackRescaleTo100ns(rawTimestamp, m_videoTimeBase);
        if (timestamp100ns >= m_videoStartTime100ns)
        {
            timestamp100ns -= m_videoStartTime100ns;
        }
        else if (timestamp100ns < 0)
        {
            timestamp100ns = m_nextVideoTimestamp100ns;
        }
        else
        {
            timestamp100ns = 0;
        }

        int64_t duration100ns = -1;
        if (frame->duration > 0)
        {
            duration100ns = playbackRescaleTo100ns(frame->duration, m_videoTimeBase);
        }
        if (duration100ns <= 0 && frame->pkt_duration > 0)
        {
            duration100ns = playbackRescaleTo100ns(frame->pkt_duration, m_videoTimeBase);
        }
        if (duration100ns <= 0)
        {
            duration100ns = m_videoFrameDuration100ns;
        }

        playbackFrame.videoFrame.timestamp100ns = playbackClampPosition(timestamp100ns, m_duration100ns);
        playbackFrame.videoFrame.duration100ns = duration100ns > 0 ? duration100ns : m_videoFrameDuration100ns;
        m_nextVideoTimestamp100ns = playbackFrame.videoFrame.timestamp100ns + playbackFrame.videoFrame.duration100ns;

        AVPixelFormat sourceFormat = static_cast<AVPixelFormat>(frame->format);
        m_scaleContext = sws_getCachedContext(m_scaleContext,
                                              frame->width,
                                              frame->height,
                                              sourceFormat,
                                              frame->width,
                                              frame->height,
                                              AV_PIX_FMT_BGRA,
                                              SWS_BILINEAR,
                                              nullptr,
                                              nullptr,
                                              nullptr);
        if (m_scaleContext == nullptr)
        {
            m_lastError = "sws_getCachedContext failed";
            return false;
        }

        const int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_BGRA, frame->width, frame->height, 1);
        if (bufferSize <= 0)
        {
            m_lastError = "av_image_get_buffer_size failed";
            return false;
        }
        playbackFrame.videoFrame.bgraPixels.resize(static_cast<size_t>(bufferSize));

        uint8_t* dstData[4] = { 0 };
        int dstLineSize[4] = { 0 };
        int result = av_image_fill_arrays(dstData,
                                          dstLineSize,
                                          &playbackFrame.videoFrame.bgraPixels[0],
                                          AV_PIX_FMT_BGRA,
                                          frame->width,
                                          frame->height,
                                          1);
        if (result < 0)
        {
            m_lastError = playbackAppendError("av_image_fill_arrays failed", result);
            return false;
        }

        result = sws_scale(m_scaleContext,
                           frame->data,
                           frame->linesize,
                           0,
                           frame->height,
                           dstData,
                           dstLineSize);
        if (result <= 0)
        {
            m_lastError = "sws_scale failed";
            return false;
        }

        ++m_videoFrameCount;
        m_frameQueue.push_back(playbackFrame);
        return true;
    }

    bool queueAudioFrame(const AVFrame* frame)
    {
        if (frame == nullptr || m_audioCodecContext == nullptr || m_resampleContext == nullptr)
        {
            return true;
        }
        if (frame->nb_samples <= 0)
        {
            return true;
        }

        const int inputSampleRate = m_audioCodecContext->sample_rate > 0 ? m_audioCodecContext->sample_rate : m_audioSampleRate;
        int64_t delay = swr_get_delay(m_resampleContext, inputSampleRate);
        int outputSampleCount = static_cast<int>(av_rescale_rnd(delay + frame->nb_samples,
                                                                m_audioSampleRate,
                                                                inputSampleRate,
                                                                AV_ROUND_UP));
        if (outputSampleCount <= 0)
        {
            return true;
        }

        int outputLineSize = 0;
        int bufferSize = av_samples_get_buffer_size(&outputLineSize,
                                                    m_audioChannels,
                                                    outputSampleCount,
                                                    AV_SAMPLE_FMT_S16,
                                                    1);
        if (bufferSize <= 0)
        {
            m_lastError = "av_samples_get_buffer_size failed";
            return false;
        }

        std::vector<unsigned char> outputBuffer;
        outputBuffer.resize(static_cast<size_t>(bufferSize));
        uint8_t* outputData[1] = { &outputBuffer[0] };
        const uint8_t** inputData = const_cast<const uint8_t**>(frame->extended_data);
        int convertedSamples = swr_convert(m_resampleContext,
                                           outputData,
                                           outputSampleCount,
                                           inputData,
                                           frame->nb_samples);
        if (convertedSamples < 0)
        {
            m_lastError = playbackAppendError("swr_convert failed", convertedSamples);
            return false;
        }
        if (convertedSamples == 0)
        {
            return true;
        }

        const int bytesPerSample = 2;
        const int usedBytes = convertedSamples * m_audioChannels * bytesPerSample;
        if (usedBytes <= 0)
        {
            return true;
        }

        FFmpegCppPlaybackFrame playbackFrame;
        playbackFrame.type = FFmpegCppPlaybackFrameTypeAudio;
        playbackFrame.audioFrame.sampleRate = m_audioSampleRate;
        playbackFrame.audioFrame.channels = m_audioChannels;
        playbackFrame.audioFrame.bitsPerSample = m_audioBitsPerSample;

        const int64_t rawTimestamp = playbackBestFrameTimestamp(frame);
        int64_t timestamp100ns = playbackRescaleTo100ns(rawTimestamp, m_audioTimeBase);
        if (timestamp100ns >= m_audioStartTime100ns)
        {
            timestamp100ns -= m_audioStartTime100ns;
        }
        else if (timestamp100ns < 0)
        {
            timestamp100ns = m_nextAudioTimestamp100ns;
        }
        else
        {
            timestamp100ns = 0;
        }

        playbackFrame.audioFrame.timestamp100ns = playbackClampPosition(timestamp100ns, m_duration100ns);
        playbackFrame.audioFrame.duration100ns = convertedSamples * kPlaybackOneSecond100ns / m_audioSampleRate;
        playbackFrame.audioFrame.pcmData.assign(outputBuffer.begin(), outputBuffer.begin() + usedBytes);
        m_nextAudioTimestamp100ns = playbackFrame.audioFrame.timestamp100ns + playbackFrame.audioFrame.duration100ns;

        ++m_audioFrameCount;
        m_frameQueue.push_back(playbackFrame);
        return true;
    }

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_filePath;
    std::string m_lastError;
    FFmpegCppMediaInfo m_mediaInfo;
    FFmpegCppPlaybackOpenOption m_option;
    std::deque<FFmpegCppPlaybackFrame> m_frameQueue;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    AVFormatContext* m_formatContext;
    AVCodecContext* m_videoCodecContext;
    AVCodecContext* m_audioCodecContext;
    SwsContext* m_scaleContext;
    SwrContext* m_resampleContext;
    AVPacket* m_packet;
    AVFrame* m_frame;
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    AVRational m_videoTimeBase;
    AVRational m_audioTimeBase;
    int64_t m_videoStartTime100ns;
    int64_t m_audioStartTime100ns;
    int64_t m_duration100ns;
    int64_t m_videoFrameDuration100ns;
    int64_t m_nextVideoTimestamp100ns;
    int64_t m_nextAudioTimestamp100ns;
    int32_t m_videoWidth;
    int32_t m_videoHeight;
    int32_t m_audioSampleRate;
    int32_t m_audioChannels;
    int32_t m_audioBitsPerSample;
    bool m_isOpen;
    bool m_drainStarted;
    bool m_videoDrained;
    bool m_audioDrained;
    uint64_t m_readPacketCount;
    uint64_t m_videoFrameCount;
    uint64_t m_audioFrameCount;
};

FFmpegCppPlaybackOpenOption::FFmpegCppPlaybackOpenOption()
    : decodeVideo(true),
      decodeAudio(true),
      outputSampleRate(0),
      outputChannels(2),
      outputBitsPerSample(16),
      fallbackVideoFrameDuration100ns(kPlaybackDefaultFrameDuration100ns)
{
}

FFmpegCppPlaybackVideoFrame::FFmpegCppPlaybackVideoFrame()
    : width(0),
      height(0),
      stride(0),
      timestamp100ns(0),
      duration100ns(kPlaybackDefaultFrameDuration100ns),
      keyFrame(false)
{
}

void FFmpegCppPlaybackVideoFrame::reset()
{
    bgraPixels.clear();
    width = 0;
    height = 0;
    stride = 0;
    timestamp100ns = 0;
    duration100ns = kPlaybackDefaultFrameDuration100ns;
    keyFrame = false;
}

bool FFmpegCppPlaybackVideoFrame::empty() const
{
    return bgraPixels.empty() || width <= 0 || height <= 0 || stride <= 0;
}

FFmpegCppPlaybackAudioFrame::FFmpegCppPlaybackAudioFrame()
    : sampleRate(0),
      channels(0),
      bitsPerSample(16),
      timestamp100ns(0),
      duration100ns(0)
{
}

void FFmpegCppPlaybackAudioFrame::reset()
{
    pcmData.clear();
    sampleRate = 0;
    channels = 0;
    bitsPerSample = 16;
    timestamp100ns = 0;
    duration100ns = 0;
}

bool FFmpegCppPlaybackAudioFrame::empty() const
{
    return pcmData.empty() || sampleRate <= 0 || channels <= 0 || bitsPerSample <= 0;
}

FFmpegCppPlaybackFrame::FFmpegCppPlaybackFrame()
    : type(FFmpegCppPlaybackFrameTypeNone)
{
}

void FFmpegCppPlaybackFrame::reset()
{
    type = FFmpegCppPlaybackFrameTypeNone;
    videoFrame.reset();
    audioFrame.reset();
}

FFmpegCppPlaybackReader::FFmpegCppPlaybackReader()
    : m_impl(new(std::nothrow) FFmpegCppPlaybackReaderImpl())
{
}

FFmpegCppPlaybackReader::~FFmpegCppPlaybackReader()
{
    close();
    FFmpegCppPlaybackReaderImpl* impl = static_cast<FFmpegCppPlaybackReaderImpl*>(m_impl);
    delete impl;
    m_impl = nullptr;
}

bool FFmpegCppPlaybackReader::open(const std::string& filePath, const FFmpegCppPlaybackOpenOption& option)
{
    FFmpegCppPlaybackReaderImpl* impl = static_cast<FFmpegCppPlaybackReaderImpl*>(m_impl);
    if (impl == nullptr)
    {
        return false;
    }

    try
    {
        return impl->open(filePath, option);
    }
    catch (const std::exception& error)
    {
        impl->m_lastError = error.what();
    }
    catch (...)
    {
        impl->m_lastError = "unknown exception in FFmpegCppPlaybackReader::open";
    }
    impl->close();
    return false;
}

void FFmpegCppPlaybackReader::close()
{
    FFmpegCppPlaybackReaderImpl* impl = static_cast<FFmpegCppPlaybackReaderImpl*>(m_impl);
    if (impl != nullptr)
    {
        impl->close();
    }
}

bool FFmpegCppPlaybackReader::isOpen() const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr && impl->isOpen();
}

std::string FFmpegCppPlaybackReader::lastError() const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr ? impl->m_lastError : std::string("FFmpegCppPlaybackReader impl is null");
}

bool FFmpegCppPlaybackReader::getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr && impl->getMediaInfo(mediaInfo);
}

int64_t FFmpegCppPlaybackReader::duration100ns() const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr ? impl->m_duration100ns : 0;
}

int64_t FFmpegCppPlaybackReader::videoFrameDuration100ns() const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr ? impl->m_videoFrameDuration100ns : kPlaybackDefaultFrameDuration100ns;
}

bool FFmpegCppPlaybackReader::getVideoSize(int32_t* width, int32_t* height) const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr && impl->getVideoSize(width, height);
}

bool FFmpegCppPlaybackReader::getAudioFormat(int32_t* sampleRate, int32_t* channels, int32_t* bitsPerSample) const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr && impl->getAudioFormat(sampleRate, channels, bitsPerSample);
}

bool FFmpegCppPlaybackReader::hasVideo() const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr && impl->m_videoStreamIndex >= 0 && impl->m_videoCodecContext != nullptr;
}

bool FFmpegCppPlaybackReader::hasAudio() const
{
    const FFmpegCppPlaybackReaderImpl* impl = static_cast<const FFmpegCppPlaybackReaderImpl*>(m_impl);
    return impl != nullptr && impl->m_audioStreamIndex >= 0 && impl->m_audioCodecContext != nullptr;
}

bool FFmpegCppPlaybackReader::seek(int64_t position100ns)
{
    FFmpegCppPlaybackReaderImpl* impl = static_cast<FFmpegCppPlaybackReaderImpl*>(m_impl);
    if (impl == nullptr)
    {
        return false;
    }

    try
    {
        return impl->seek(position100ns);
    }
    catch (const std::exception& error)
    {
        impl->m_lastError = error.what();
    }
    catch (...)
    {
        impl->m_lastError = "unknown exception in FFmpegCppPlaybackReader::seek";
    }
    return false;
}

FFmpegCppPlaybackReadResult FFmpegCppPlaybackReader::read(FFmpegCppPlaybackFrame* frame)
{
    FFmpegCppPlaybackReaderImpl* impl = static_cast<FFmpegCppPlaybackReaderImpl*>(m_impl);
    if (impl == nullptr)
    {
        return FFmpegCppPlaybackReadResultError;
    }

    try
    {
        return impl->read(frame);
    }
    catch (const std::exception& error)
    {
        impl->m_lastError = error.what();
    }
    catch (...)
    {
        impl->m_lastError = "unknown exception in FFmpegCppPlaybackReader::read";
    }
    return FFmpegCppPlaybackReadResultError;
}

bool FFmpegCppPlaybackReader::readVideoFrameAt(int64_t position100ns, size_t maxReadFrameCount, FFmpegCppPlaybackVideoFrame* frame)
{
    FFmpegCppPlaybackReaderImpl* impl = static_cast<FFmpegCppPlaybackReaderImpl*>(m_impl);
    if (impl == nullptr)
    {
        return false;
    }

    try
    {
        return impl->readVideoFrameAt(position100ns, maxReadFrameCount, frame);
    }
    catch (const std::exception& error)
    {
        impl->m_lastError = error.what();
    }
    catch (...)
    {
        impl->m_lastError = "unknown exception in FFmpegCppPlaybackReader::readVideoFrameAt";
    }
    return false;
}