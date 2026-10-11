#include "FFmpegCppMediaWriter.h"
#include <algorithm>
#include <cstring>
#include "FFmpegCppNativeMediaWriter.h"
extern "C"
{
#include "ffmpeg/libavformat/avformat.h"
#include "ffmpeg/libavcodec/avcodec.h"
#include "ffmpeg/libavutil/opt.h"
#include "ffmpeg/libswresample/swresample.h"
#include "ffmpeg/libswscale/swscale.h"
}

FFmpegCppMediaWriter::FFmpegCppMediaWriter() : m_format(nullptr), m_audio(nullptr),
    m_video(nullptr), m_audioStream(nullptr), m_videoStream(nullptr), m_resampler(nullptr),
    m_scaler(nullptr), m_audioFrames(0), m_started(false), m_native(nullptr), m_width(0), m_height(0) {}
FFmpegCppMediaWriter::~FFmpegCppMediaWriter() { close(); }
void FFmpegCppMediaWriter::close()
{
#ifdef _WIN32
    delete m_native;
    m_native = nullptr;
#endif
    swr_free(&m_resampler);
    sws_freeContext(m_scaler);
    m_scaler = nullptr;
    avcodec_free_context(&m_audio);
    avcodec_free_context(&m_video);
    if (m_format)
    {
        if (m_format->pb) avio_closep(&m_format->pb);
        avformat_free_context(m_format);
        m_format = nullptr;
    }
    m_started = false;
}
bool FFmpegCppMediaWriter::fail(int code, const char* stage)
{
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, text, sizeof(text));
    m_error = std::string(stage) + ": " + text;
    return false;
}
std::string FFmpegCppMediaWriter::error() const { return m_error; }
bool FFmpegCppMediaWriter::open(const std::string& path, bool withVideo, int width, int height)
{
    if (m_format || m_native || path.empty() || (withVideo && (width < 2 || height < 2 || width > 16384 ||
        height > 16384 || width % 2 || height % 2))) return fail(AVERROR(EINVAL), "invalid writer settings");
    m_error.clear();
    m_audioFrames = 0;
    m_pending.clear();
    m_width = withVideo ? width : 0;
    m_height = withVideo ? height : 0;
#ifdef _WIN32
    // Select before creating any output; an encoding/write error must never silently restart a file.
    if (!avcodec_find_encoder(withVideo ? AV_CODEC_ID_AAC : AV_CODEC_ID_MP3) ||
        (withVideo && !avcodec_find_encoder(AV_CODEC_ID_H264)))
    {
        m_native = new FFmpegCppNativeMediaWriter;
        m_started = m_native->open(path, withVideo, width, height);
        if (!m_started) { m_error = m_native->error(); close(); }
        return m_started;
    }
#endif
    int result = avformat_alloc_output_context2(&m_format, nullptr, withVideo ? "mp4" : "mp3", path.c_str());
    if (result < 0) return fail(result, "create muxer");
    for (int kind = 0; kind < (withVideo ? 2 : 1); ++kind)
    {
        const AVCodecID id = kind ? AV_CODEC_ID_H264 : (withVideo ? AV_CODEC_ID_AAC : AV_CODEC_ID_MP3);
        const AVCodec* encoder = avcodec_find_encoder_by_name(kind ? "libx264" : (withVideo ? "aac" : "libmp3lame"));
        if (!encoder) encoder = avcodec_find_encoder(id);
        if (!encoder) return fail(AVERROR_ENCODER_NOT_FOUND, "required H264/AAC/MP3 encoder unavailable");
        AVCodecContext*& codec = kind ? m_video : m_audio;
        AVStream*& stream = kind ? m_videoStream : m_audioStream;
        codec = avcodec_alloc_context3(encoder);
        stream = avformat_new_stream(m_format, nullptr);
        if (!codec || !stream) return fail(AVERROR(ENOMEM), "allocate encoder");
        codec->bit_rate = kind ? 8000000 : 192000;
        if (kind)
        {
            codec->width = width;
            codec->height = height;
            codec->pix_fmt = AV_PIX_FMT_YUV420P;
            codec->time_base = AVRational{1, 30};
            codec->framerate = AVRational{30, 1};
            codec->gop_size = 60;
            codec->max_b_frames = 0;
        }
        else
        {
            codec->sample_rate = 48000;
            av_channel_layout_default(&codec->ch_layout, 2);
            codec->sample_fmt = encoder->sample_fmts ? encoder->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            codec->time_base = AVRational{1, 48000};
        }
        if (m_format->oformat->flags & AVFMT_GLOBALHEADER) codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        result = avcodec_open2(codec, encoder, nullptr);
        if (result < 0) return fail(result, "open encoder");
        result = avcodec_parameters_from_context(stream->codecpar, codec);
        if (result < 0) return fail(result, "copy stream parameters");
        stream->time_base = codec->time_base;
    }
    AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
    result = swr_alloc_set_opts2(&m_resampler, &m_audio->ch_layout, m_audio->sample_fmt, 48000,
        &stereo, AV_SAMPLE_FMT_S16, 48000, 0, nullptr);
    if (result >= 0) result = swr_init(m_resampler);
    if (result < 0) return fail(result, "initialize PCM conversion");
    result = avio_open(&m_format->pb, path.c_str(), AVIO_FLAG_WRITE);
    if (result >= 0) result = avformat_write_header(m_format, nullptr);
    if (result < 0) return fail(result, "open output");
    m_started = true;
    return true;
}
bool FFmpegCppMediaWriter::encode(AVCodecContext* codec, AVStream* stream, AVFrame* frame)
{
    int result = avcodec_send_frame(codec, frame);
    if (result < 0) return fail(result, "send encoder frame");
    AVPacket* packet = av_packet_alloc();
    if (!packet) return fail(AVERROR(ENOMEM), "allocate packet");
    for (;;)
    {
        result = avcodec_receive_packet(codec, packet);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) { result = 0; break; }
        if (result < 0) break;
        av_packet_rescale_ts(packet, codec->time_base, stream->time_base);
        packet->stream_index = stream->index;
        result = av_interleaved_write_frame(m_format, packet);
        av_packet_unref(packet);
        if (result < 0) break;
    }
    av_packet_free(&packet);
    return result >= 0 || fail(result, "write encoded packet");
}
bool FFmpegCppMediaWriter::audioBlock(size_t count)
{
    AVFrame* frame = av_frame_alloc();
    if (!frame) return fail(AVERROR(ENOMEM), "allocate audio frame");
    frame->format = m_audio->sample_fmt;
    frame->sample_rate = 48000;
    frame->nb_samples = static_cast<int>(count);
    frame->pts = m_audioFrames;
    int result = av_channel_layout_copy(&frame->ch_layout, &m_audio->ch_layout);
    if (result >= 0) result = av_frame_get_buffer(frame, 0);
    const unsigned char* input[] = { reinterpret_cast<const unsigned char*>(m_pending.data()) };
    if (result >= 0) result = swr_convert(m_resampler, frame->data, frame->nb_samples, input, frame->nb_samples);
    const bool success = result == frame->nb_samples && encode(m_audio, m_audioStream, frame);
    if (result < 0) fail(result, "convert audio frame");
    av_frame_free(&frame);
    if (!success) return false;
    m_audioFrames += count;
    m_pending.erase(m_pending.begin(), m_pending.begin() + count * 2);
    return true;
}
bool FFmpegCppMediaWriter::audio(const int16_t* samples, size_t frames)
{
    if (!m_started || !samples || frames == 0 || frames > 48000) return fail(AVERROR(EINVAL), "invalid audio block");
#ifdef _WIN32
    if (m_native)
    {
        if (!m_native->audio(samples, frames, m_audioFrames * 10000000LL / 48000))
        { m_error = m_native->error(); return false; }
        m_audioFrames += frames;
        return true;
    }
#endif
    m_pending.insert(m_pending.end(), samples, samples + frames * 2);
    const size_t block = m_audio->frame_size > 0 ? m_audio->frame_size : 1024;
    while (m_pending.size() / 2 >= block) if (!audioBlock(block)) return false;
    return true;
}
bool FFmpegCppMediaWriter::video(const unsigned char* pixels, size_t bytes, int64_t time100ns)
{
    if (!m_started || m_width == 0 || !pixels || time100ns < 0 ||
        bytes != static_cast<size_t>(m_width) * m_height * 4) return fail(AVERROR(EINVAL), "invalid video frame");
#ifdef _WIN32
    if (m_native)
    {
        if (m_native->video(pixels, bytes, time100ns, 10000000LL / 30)) return true;
        m_error = m_native->error();
        return false;
    }
#endif
    AVFrame* frame = av_frame_alloc();
    if (!frame) return fail(AVERROR(ENOMEM), "allocate video frame");
    frame->format = m_video->pix_fmt;
    frame->width = m_video->width;
    frame->height = m_video->height;
    frame->pts = av_rescale_q(time100ns, AVRational{1, 10000000}, m_video->time_base);
    int result = av_frame_get_buffer(frame, 32);
    if (result >= 0)
    {
        m_scaler = sws_getCachedContext(m_scaler, frame->width, frame->height, AV_PIX_FMT_BGRA,
            frame->width, frame->height, m_video->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!m_scaler) result = AVERROR(ENOMEM);
        else
        {
            const unsigned char* data[] = {pixels};
            const int strides[] = {frame->width * 4};
            result = sws_scale(m_scaler, data, strides, 0, frame->height, frame->data, frame->linesize);
        }
    }
    const bool success = result >= 0 && encode(m_video, m_videoStream, frame);
    if (result < 0) fail(result, "convert video frame");
    av_frame_free(&frame);
    return success;
}
bool FFmpegCppMediaWriter::finish()
{
    if (!m_started) return fail(AVERROR(EINVAL), "writer not started");
#ifdef _WIN32
    if (m_native)
    {
        const bool result = m_native->finish();
        if (!result) m_error = m_native->error();
        delete m_native;
        m_native = nullptr;
        m_started = false;
        return result;
    }
#endif
    if (!m_pending.empty() && !audioBlock(m_pending.size() / 2)) return false;
    if (!encode(m_audio, m_audioStream, nullptr) || (m_video && !encode(m_video, m_videoStream, nullptr))) return false;
    const int result = av_write_trailer(m_format);
    m_started = false;
    if (result < 0) return fail(result, "finish output");
    const int closed = avio_closep(&m_format->pb);
    if (closed < 0) return fail(closed, "close output");
    return true;
}