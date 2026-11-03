#include "FFmpegCppAudioTranscoder.h"
#include <algorithm>
extern "C"
{
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavutil/audio_fifo.h>
#include <ffmpeg/libswresample/swresample.h>
}

FFmpegCppAudioTranscoder::FFmpegCppAudioTranscoder() :
m_output(nullptr),
m_inputStream(nullptr),
m_outputStream(nullptr),
m_decoder(nullptr),
m_encoder(nullptr),
m_resampler(nullptr),
m_fifo(nullptr),
m_streamIndex(-1),
m_nextPts(0),
m_timed(false)
{

}

FFmpegCppAudioTranscoder::~FFmpegCppAudioTranscoder()
{
    if (m_fifo != nullptr)
    {
        av_audio_fifo_free(m_fifo);
    }
    swr_free(&m_resampler);
    avcodec_free_context(&m_decoder);
    avcodec_free_context(&m_encoder);
}

bool FFmpegCppAudioTranscoder::open(AVFormatContext* input, AVFormatContext* output, int32_t bitRateKbps, std::string& error)
{
    m_output = output;
    m_streamIndex = av_find_best_stream(input, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_streamIndex < 0)
    {
        return true;
    }
    m_inputStream = input->streams[m_streamIndex];
    const AVCodec* decoder = avcodec_find_decoder(m_inputStream->codecpar->codec_id);
    const AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (decoder == nullptr || encoder == nullptr || bitRateKbps < 16 || bitRateKbps > 512)
    {
        error = "required AAC encoder or input audio decoder is unavailable";
        return false;
    }
    m_decoder = avcodec_alloc_context3(decoder);
    m_encoder = avcodec_alloc_context3(encoder);
    if (m_decoder == nullptr || m_encoder == nullptr || avcodec_parameters_to_context(m_decoder, m_inputStream->codecpar) < 0)
    {
        error = "cannot allocate audio transcoder";
        return false;
    }
    m_decoder->thread_count = 1;
    if (avcodec_open2(m_decoder, decoder, nullptr) < 0)
    {
        error = "cannot open input audio decoder";
        return false;
    }
    m_encoder->sample_rate = 48000;
    m_encoder->sample_fmt = AV_SAMPLE_FMT_FLTP;
    m_encoder->time_base = AVRational{1, 48000};
    m_encoder->bit_rate = static_cast<int64_t>(bitRateKbps) * 1000;
    m_encoder->thread_count = 1;
    av_channel_layout_default(&m_encoder->ch_layout, m_decoder->ch_layout.nb_channels == 1 ? 1 : 2);
    if (output->oformat->flags & AVFMT_GLOBALHEADER)
    {
        m_encoder->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if (avcodec_open2(m_encoder, encoder, nullptr) < 0)
    {
        error = "cannot open AAC encoder";
        return false;
    }
    m_outputStream = avformat_new_stream(output, nullptr);
    if (m_outputStream == nullptr || avcodec_parameters_from_context(m_outputStream->codecpar, m_encoder) < 0)
    {
        error = "cannot create AAC output stream";
        return false;
    }
    m_outputStream->time_base = m_encoder->time_base;
    m_outputStream->codecpar->codec_tag = 0;
    const int32_t created = swr_alloc_set_opts2(&m_resampler, &m_encoder->ch_layout, m_encoder->sample_fmt,
        m_encoder->sample_rate, &m_decoder->ch_layout, m_decoder->sample_fmt, m_decoder->sample_rate, 0, nullptr);
    m_fifo = av_audio_fifo_alloc(m_encoder->sample_fmt, m_encoder->ch_layout.nb_channels, 4096);
    if (created < 0 || m_resampler == nullptr || swr_init(m_resampler) < 0 || m_fifo == nullptr)
    {
        error = "cannot initialize bounded audio conversion";
        return false;
    }
    return true;
}

bool FFmpegCppAudioTranscoder::encode(AVFrame* frame, std::string& error)
{
    int32_t result = avcodec_send_frame(m_encoder, frame);
    if (result < 0)
    {
        error = "AAC encoder rejected samples";
        return false;
    }
    AVPacket* packet = av_packet_alloc();
    if (packet == nullptr)
    {
        error = "cannot allocate AAC packet";
        return false;
    }
    while ((result = avcodec_receive_packet(m_encoder, packet)) >= 0)
    {
        av_packet_rescale_ts(packet, m_encoder->time_base, m_outputStream->time_base);
        packet->stream_index = m_outputStream->index;
        result = av_interleaved_write_frame(m_output, packet);
        av_packet_unref(packet);
        if (result < 0)
        {
            break;
        }
    }
    av_packet_free(&packet);
    if (result != AVERROR(EAGAIN) && result != AVERROR_EOF)
    {
        error = "cannot write AAC packet";
        return false;
    }
    return true;
}

bool FFmpegCppAudioTranscoder::drain(bool final, std::string& error)
{
    const int32_t frameSize = m_encoder->frame_size;
    if (frameSize <= 0)
    {
        error = "AAC encoder reported invalid frame size";
        return false;
    }
    while (av_audio_fifo_size(m_fifo) >= frameSize || (final && av_audio_fifo_size(m_fifo) > 0))
    {
        AVFrame* frame = av_frame_alloc();
        if (frame == nullptr)
        {
            error = "cannot allocate AAC frame";
            return false;
        }
        frame->format = m_encoder->sample_fmt;
        frame->sample_rate = m_encoder->sample_rate;
        frame->nb_samples = (std::min)(frameSize, av_audio_fifo_size(m_fifo));
        const bool allocated = av_channel_layout_copy(&frame->ch_layout, &m_encoder->ch_layout) >= 0 && av_frame_get_buffer(frame, 0) >= 0;
        bool success = allocated && av_audio_fifo_read(m_fifo, reinterpret_cast<void**>(frame->data), frame->nb_samples) == frame->nb_samples;
        if (success)
        {
            frame->pts = m_nextPts;
            m_nextPts += frame->nb_samples;
            success = encode(frame, error);
        }
        av_frame_free(&frame);
        if (!success)
        {
            if (error.empty())
            {
                error = "cannot read converted audio samples";
            }
            return false;
        }
    }
    return true;
}

bool FFmpegCppAudioTranscoder::convert(AVFrame* frame, std::string& error)
{
    if (!m_timed && frame != nullptr)
    {
        const int64_t pts = frame->best_effort_timestamp;
        m_nextPts = pts == AV_NOPTS_VALUE ? 0 : av_rescale_q(pts, m_inputStream->time_base, m_encoder->time_base);
        m_timed = true;
    }
    const int64_t delayed = swr_get_delay(m_resampler, m_decoder->sample_rate);
    const int64_t wanted = av_rescale_rnd(delayed + (frame == nullptr ? 0 : frame->nb_samples), 48000,
        m_decoder->sample_rate, AV_ROUND_UP);
    if (wanted > 1048576 || wanted < 0)
    {
        error = "audio conversion exceeds sample budget";
        return false;
    }
    if (wanted == 0)
    {
        return true;
    }
    uint8_t** converted = nullptr;
    int32_t stride = 0;
    int32_t result = av_samples_alloc_array_and_samples(&converted, &stride, m_encoder->ch_layout.nb_channels,
        static_cast<int32_t>(wanted), m_encoder->sample_fmt, 0);
    if (result < 0)
    {
        error = "cannot allocate audio conversion buffer";
        return false;
    }
    result = swr_convert(m_resampler, converted, static_cast<int32_t>(wanted),
        frame == nullptr ? nullptr : const_cast<const uint8_t**>(frame->extended_data), frame == nullptr ? 0 : frame->nb_samples);
    const bool success = result >= 0 && av_audio_fifo_realloc(m_fifo, av_audio_fifo_size(m_fifo) + result) >= 0 &&
        av_audio_fifo_write(m_fifo, reinterpret_cast<void**>(converted), result) == result;
    av_freep(&converted[0]);
    av_freep(&converted);
    if (!success)
    {
        error = "cannot resample input audio";
        return false;
    }
    return drain(false, error);
}

bool FFmpegCppAudioTranscoder::receive(std::string& error)
{
    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr)
    {
        error = "cannot allocate decoded audio frame";
        return false;
    }
    int32_t result = 0;
    bool success = true;
    while ((result = avcodec_receive_frame(m_decoder, frame)) >= 0)
    {
        success = convert(frame, error);
        av_frame_unref(frame);
        if (!success)
        {
            break;
        }
    }
    av_frame_free(&frame);
    return success && (result == AVERROR(EAGAIN) || result == AVERROR_EOF);
}

bool FFmpegCppAudioTranscoder::process(AVPacket* packet, std::string& error)
{
    if (m_streamIndex < 0 || packet->stream_index != m_streamIndex)
    {
        return true;
    }
    if (avcodec_send_packet(m_decoder, packet) < 0 || !receive(error))
    {
        if (error.empty())
        {
            error = "cannot decode input audio packet";
        }
        return false;
    }
    return true;
}

bool FFmpegCppAudioTranscoder::finish(std::string& error)
{
    if (m_streamIndex < 0)
    {
        return true;
    }
    return avcodec_send_packet(m_decoder, nullptr) >= 0 && receive(error) && convert(nullptr, error) &&
        drain(true, error) && encode(nullptr, error);
}