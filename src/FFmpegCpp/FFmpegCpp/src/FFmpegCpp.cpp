#include "FFmpegCpp.h"
#include "FFmpegCppHelper.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdio.h>

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
#include <ffmpeg/libavutil/dict.h>
#include <ffmpeg/libavutil/display.h>
#include <ffmpeg/libavutil/error.h>
#include <ffmpeg/libavutil/imgutils.h>
#include <ffmpeg/libavutil/opt.h>
#include <ffmpeg/libavutil/pixdesc.h>
#include <ffmpeg/libswscale/swscale.h>
}

static std::string ffmpegCppErrorText(int errorCode)
{
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(errorCode, buffer, sizeof(buffer)) == 0)
    {
        return buffer;
    }

    std::ostringstream stream;
    stream << "ffmpeg error " << errorCode;
    return stream.str();
}

static std::string ffmpegCppAppendError(const std::string& prefix, int errorCode)
{
    std::ostringstream stream;
    stream << prefix << ": " << ffmpegCppErrorText(errorCode);
    return stream.str();
}

static int64_t ffmpegCppToMilliseconds(int64_t timestamp, AVRational timeBase)
{
    if (timestamp == AV_NOPTS_VALUE)
    {
        return 0;
    }

    AVRational millisecondBase;
    millisecondBase.num = 1;
    millisecondBase.den = 1000;
    return av_rescale_q(timestamp, timeBase, millisecondBase);
}

static int64_t ffmpegCppFromMilliseconds(int64_t timestampMilliseconds, AVRational timeBase)
{
    AVRational millisecondBase;
    millisecondBase.num = 1;
    millisecondBase.den = 1000;
    return av_rescale_q(timestampMilliseconds, millisecondBase, timeBase);
}

static bool ffmpegCppFileExists(const std::string& filePath)
{
    std::ifstream stream(filePath.c_str(), std::ios::binary);
    return stream.good();
}

static int64_t ffmpegCppFileSize(const std::string& filePath)
{
    std::ifstream stream(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!stream.good())
    {
        return 0;
    }
    return static_cast<int64_t>(stream.tellg());
}

static std::string ffmpegCppRemoveExtension(const std::string& filePath)
{
    size_t slashPos = filePath.find_last_of("/\\");
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return filePath;
    }
    if (slashPos != std::string::npos && dotPos < slashPos)
    {
        return filePath;
    }
    return filePath.substr(0, dotPos);
}

static std::string ffmpegCppMakeFrameFilePath(const std::string& sourceFilePath,
                                       int64_t timestampMilliseconds,
                                       FFmpegCppImageFormat imageFormat)
{
    std::ostringstream stream;
    stream << ffmpegCppRemoveExtension(sourceFilePath)
           << "_"
           << timestampMilliseconds
           << "."
           << FFmpegCppHelper::imageFormatToExtension(imageFormat);
    return stream.str();
}

static bool ffmpegCppTimeoutReached(const std::chrono::steady_clock::time_point& startTime,
                             int32_t timeoutMilliseconds)
{
    if (timeoutMilliseconds <= 0)
    {
        return false;
    }

    std::chrono::steady_clock::time_point nowTime = std::chrono::steady_clock::now();
    int64_t usedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime).count();
    return usedMilliseconds >= timeoutMilliseconds;
}

static FFmpegCppMediaType ffmpegCppConvertMediaType(AVMediaType mediaType)
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

static FFmpegCppRational ffmpegCppConvertRational(AVRational rational)
{
    return FFmpegCppRational(rational.num, rational.den);
}

static int32_t ffmpegCppStreamRotationDegree(const AVStream* stream)
{
    if (stream == nullptr)
    {
        return 0;
    }

    AVDictionaryEntry* rotate = av_dict_get(stream->metadata, "rotate", nullptr, 0);
    if (rotate != nullptr && rotate->value != nullptr)
    {
        return atoi(rotate->value);
    }

    size_t displayMatrixSize = 0;
    const uint8_t* displayMatrix = av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, &displayMatrixSize);
    if (displayMatrix == nullptr || displayMatrixSize == 0)
    {
        return 0;
    }

    double rotation = -av_display_rotation_get(reinterpret_cast<const int32_t*>(displayMatrix));
    if (rotation > 180.0)
    {
        rotation -= 360.0;
    }
    else if (rotation <= -180.0)
    {
        rotation += 360.0;
    }
    return static_cast<int32_t>(rotation);
}

static std::string ffmpegCppDictionaryValue(AVDictionary* dictionary, const char* key)
{
    if (dictionary == nullptr || key == nullptr)
    {
        return std::string();
    }

    AVDictionaryEntry* entry = av_dict_get(dictionary, key, nullptr, 0);
    if (entry == nullptr || entry->value == nullptr)
    {
        return std::string();
    }
    return entry->value;
}

static FFmpegCppStreamInfo ffmpegCppMakeStreamInfo(const AVFormatContext* formatContext,
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
    streamInfo.mediaType = ffmpegCppConvertMediaType(codecParameters->codec_type);
    streamInfo.codecName = codecName != nullptr ? codecName : std::string();
    streamInfo.codecLongName = codecDescriptor != nullptr && codecDescriptor->long_name != nullptr ? codecDescriptor->long_name : std::string();
    streamInfo.profile = profileName != nullptr ? profileName : std::string();
    streamInfo.width = codecParameters->width;
    streamInfo.height = codecParameters->height;
    streamInfo.rotationDegree = ffmpegCppStreamRotationDegree(stream);
    streamInfo.bitRate = codecParameters->bit_rate;
    streamInfo.timeBase = ffmpegCppConvertRational(stream->time_base);
    streamInfo.sampleRate = codecParameters->sample_rate;
    streamInfo.channels = codecParameters->ch_layout.nb_channels;
    streamInfo.language = ffmpegCppDictionaryValue(stream->metadata, "language");

    AVRational frameRate = stream->avg_frame_rate.num != 0 && stream->avg_frame_rate.den != 0 ? stream->avg_frame_rate : stream->r_frame_rate;
    streamInfo.frameRate = ffmpegCppConvertRational(frameRate);
    if (stream->duration != AV_NOPTS_VALUE)
    {
         streamInfo.durationMilliseconds = ffmpegCppToMilliseconds(stream->duration, stream->time_base);
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

static void ffmpegCppCalculateTargetSize(int32_t sourceWidth,
                                  int32_t sourceHeight,
                                  const FFmpegCppFrameExtractOption& option,
                                  int32_t& targetWidth,
                                  int32_t& targetHeight)
{
    targetWidth = sourceWidth;
    targetHeight = sourceHeight;
    if (sourceWidth <= 0 || sourceHeight <= 0)
    {
        targetWidth = 0;
        targetHeight = 0;
        return;
    }

    if (!option.keepAspectRatio)
    {
        targetWidth = option.maxWidth > 0 ? option.maxWidth : sourceWidth;
        targetHeight = option.maxHeight > 0 ? option.maxHeight : sourceHeight;
        targetWidth = std::max<int32_t>(1, targetWidth);
        targetHeight = std::max<int32_t>(1, targetHeight);
        return;
    }

    if (option.maxWidth > 0 && targetWidth > option.maxWidth)
    {
        targetWidth = option.maxWidth;
        targetHeight = static_cast<int32_t>(static_cast<int64_t>(sourceHeight) * targetWidth / sourceWidth);
    }
    if (option.maxHeight > 0 && targetHeight > option.maxHeight)
    {
        targetHeight = option.maxHeight;
        targetWidth = static_cast<int32_t>(static_cast<int64_t>(sourceWidth) * targetHeight / sourceHeight);
    }
    targetWidth = std::max<int32_t>(1, targetWidth);
    targetHeight = std::max<int32_t>(1, targetHeight);
}

static AVPixelFormat ffmpegCppImagePixelFormat(FFmpegCppImageFormat imageFormat)
{
    if (imageFormat == FFmpegCppImageFormatBmp)
    {
        return AV_PIX_FMT_BGR24;
    }
    return AV_PIX_FMT_YUVJ420P;
}

static AVCodecID ffmpegCppImageCodecId(FFmpegCppImageFormat imageFormat)
{
    if (imageFormat == FFmpegCppImageFormatBmp)
    {
        return AV_CODEC_ID_BMP;
    }
    if (imageFormat == FFmpegCppImageFormatPng)
    {
        return AV_CODEC_ID_PNG;
    }
    return AV_CODEC_ID_MJPEG;
}

static bool ffmpegCppEncodeFrameToImage(const AVFrame* sourceFrame,
                                 AVCodecContext* decodeContext,
                                 int64_t timestampMilliseconds,
                                 const FFmpegCppFrameExtractOption& option,
                                 FFmpegCppImageData* imageData,
                                 std::string& errorText)
{
    if (sourceFrame == nullptr || decodeContext == nullptr || imageData == nullptr)
    {
        errorText = "invalid frame encode parameter";
        return false;
    }
    if (option.imageFormat == FFmpegCppImageFormatPng)
    {
        errorText = "png image output is not enabled in current ffmpeg package";
        return false;
    }

    int32_t targetWidth = 0;
    int32_t targetHeight = 0;
    ffmpegCppCalculateTargetSize(sourceFrame->width, sourceFrame->height, option, targetWidth, targetHeight);
    if (targetWidth <= 0 || targetHeight <= 0)
    {
        errorText = "invalid frame size";
        return false;
    }

    AVPixelFormat sourcePixelFormat = static_cast<AVPixelFormat>(sourceFrame->format);
    if (sourcePixelFormat == AV_PIX_FMT_NONE)
    {
        sourcePixelFormat = decodeContext->pix_fmt;
    }

    AVPixelFormat targetPixelFormat = ffmpegCppImagePixelFormat(option.imageFormat);
    SwsContext* scaleContext = sws_getContext(sourceFrame->width,
                                              sourceFrame->height,
                                              sourcePixelFormat,
                                              targetWidth,
                                              targetHeight,
                                              targetPixelFormat,
                                              SWS_BILINEAR,
                                              nullptr,
                                              nullptr,
                                              nullptr);
    if (scaleContext == nullptr)
    {
        errorText = "create scale context failed";
        return false;
    }

    AVFrame* targetFrame = av_frame_alloc();
    if (targetFrame == nullptr)
    {
        sws_freeContext(scaleContext);
        errorText = "alloc target frame failed";
        return false;
    }

    targetFrame->format = targetPixelFormat;
    targetFrame->width = targetWidth;
    targetFrame->height = targetHeight;
    int imageAllocResult = av_image_alloc(targetFrame->data, targetFrame->linesize, targetWidth, targetHeight, targetPixelFormat, 32);
    if (imageAllocResult < 0)
    {
        av_frame_free(&targetFrame);
        sws_freeContext(scaleContext);
         errorText = ffmpegCppAppendError("alloc image buffer failed", imageAllocResult);
        return false;
    }

    sws_scale(scaleContext,
              sourceFrame->data,
              sourceFrame->linesize,
              0,
              sourceFrame->height,
              targetFrame->data,
              targetFrame->linesize);

    AVCodecID imageCodecId = ffmpegCppImageCodecId(option.imageFormat);
    const AVCodec* encoder = avcodec_find_encoder(imageCodecId);
    if (encoder == nullptr)
    {
        av_freep(&targetFrame->data[0]);
        av_frame_free(&targetFrame);
        sws_freeContext(scaleContext);
        errorText = "image encoder is not enabled";
        return false;
    }

    AVCodecContext* encodeContext = avcodec_alloc_context3(encoder);
    if (encodeContext == nullptr)
    {
        av_freep(&targetFrame->data[0]);
        av_frame_free(&targetFrame);
        sws_freeContext(scaleContext);
        errorText = "alloc image encoder context failed";
        return false;
    }

    encodeContext->width = targetWidth;
    encodeContext->height = targetHeight;
    encodeContext->pix_fmt = targetPixelFormat;
    encodeContext->time_base.num = 1;
    encodeContext->time_base.den = 1000;
    if (option.imageFormat == FFmpegCppImageFormatJpeg)
    {
        int jpegQuality = std::max<int32_t>(1, std::min<int32_t>(100, option.jpegQuality));
        int qscale = 31 - jpegQuality * 29 / 100;
        qscale = std::max<int32_t>(2, std::min<int32_t>(31, qscale));
        encodeContext->flags |= AV_CODEC_FLAG_QSCALE;
        encodeContext->global_quality = FF_QP2LAMBDA * qscale;
        targetFrame->quality = encodeContext->global_quality;
    }

    int result = avcodec_open2(encodeContext, encoder, nullptr);
    if (result < 0)
    {
        avcodec_free_context(&encodeContext);
        av_freep(&targetFrame->data[0]);
        av_frame_free(&targetFrame);
        sws_freeContext(scaleContext);
         errorText = ffmpegCppAppendError("open image encoder failed", result);
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    if (packet == nullptr)
    {
        avcodec_free_context(&encodeContext);
        av_freep(&targetFrame->data[0]);
        av_frame_free(&targetFrame);
        sws_freeContext(scaleContext);
        errorText = "alloc image packet failed";
        return false;
    }

    result = avcodec_send_frame(encodeContext, targetFrame);
    if (result >= 0)
    {
        result = avcodec_receive_packet(encodeContext, packet);
        if (result == AVERROR(EAGAIN))
        {
            avcodec_send_frame(encodeContext, nullptr);
            result = avcodec_receive_packet(encodeContext, packet);
        }
    }

    if (result < 0)
    {
         errorText = ffmpegCppAppendError("encode image failed", result);
        av_packet_free(&packet);
        avcodec_free_context(&encodeContext);
        av_freep(&targetFrame->data[0]);
        av_frame_free(&targetFrame);
        sws_freeContext(scaleContext);
        return false;
    }

    imageData->reset();
    imageData->imageFormat = option.imageFormat;
    imageData->mimeType = FFmpegCppHelper::imageFormatToMimeType(option.imageFormat);
    imageData->width = targetWidth;
    imageData->height = targetHeight;
    imageData->timestampMilliseconds = timestampMilliseconds;
    imageData->data.assign(packet->data, packet->data + packet->size);

    av_packet_free(&packet);
    avcodec_free_context(&encodeContext);
    av_freep(&targetFrame->data[0]);
    av_frame_free(&targetFrame);
    sws_freeContext(scaleContext);
    return true;
}

static bool ffmpegCppIsMovLikeOutput(const AVOutputFormat* outputFormat)
{
    if (outputFormat == nullptr || outputFormat->name == nullptr)
    {
        return false;
    }

    std::string outputName = outputFormat->name;
    return outputName.find("mp4") != std::string::npos
        || outputName.find("mov") != std::string::npos
        || outputName.find("ipod") != std::string::npos
        || outputName.find("3gp") != std::string::npos;
}

FFmpegCpp::FFmpegCpp()
    : m_filePath()
    , m_lastError()
    , m_mediaInfo()
    , m_isOpen(false)
    , m_cancelRequested(false)
    , m_formatContext(nullptr)
    , m_videoCodecContext(nullptr)
    , m_audioCodecContext(nullptr)
    , m_scaleContext(nullptr)
{
    av_log_set_level(AV_LOG_ERROR);
}

FFmpegCpp::~FFmpegCpp()
{
    close();
}

bool FFmpegCpp::open(const std::string& filePath)
{
    close();
    m_lastError.clear();
    if (filePath.empty())
    {
        m_lastError = "filePath is empty";
        return false;
    }

    AVFormatContext* formatContext = nullptr;
    int result = avformat_open_input(&formatContext, filePath.c_str(), nullptr, nullptr);
    if (result < 0)
    {
        m_lastError = ffmpegCppAppendError("open input failed", result);
        return false;
    }

    result = avformat_find_stream_info(formatContext, nullptr);
    if (result < 0)
    {
        avformat_close_input(&formatContext);
        m_lastError = ffmpegCppAppendError("find stream info failed", result);
        return false;
    }

    m_formatContext = formatContext;
    m_filePath = filePath;
    m_isOpen = true;
    m_cancelRequested = false;
    if (!reloadMediaInfo())
    {
        close();
        return false;
    }
    return true;
}

void FFmpegCpp::close()
{
    if (m_videoCodecContext != nullptr)
    {
        AVCodecContext* videoCodecContext = static_cast<AVCodecContext*>(m_videoCodecContext);
        avcodec_free_context(&videoCodecContext);
        m_videoCodecContext = nullptr;
    }

    if (m_audioCodecContext != nullptr)
    {
        AVCodecContext* audioCodecContext = static_cast<AVCodecContext*>(m_audioCodecContext);
        avcodec_free_context(&audioCodecContext);
        m_audioCodecContext = nullptr;
    }

    if (m_scaleContext != nullptr)
    {
        sws_freeContext(static_cast<SwsContext*>(m_scaleContext));
        m_scaleContext = nullptr;
    }

    if (m_formatContext != nullptr)
    {
        AVFormatContext* formatContext = static_cast<AVFormatContext*>(m_formatContext);
        avformat_close_input(&formatContext);
        m_formatContext = nullptr;
    }

    m_filePath.clear();
    m_mediaInfo.reset();
    m_isOpen = false;
    m_cancelRequested = false;
}

bool FFmpegCpp::isOpen() const
{
    return m_isOpen && m_formatContext != nullptr;
}

std::string FFmpegCpp::filePath() const
{
    return m_filePath;
}

std::string FFmpegCpp::lastError() const
{
    return m_lastError;
}

bool FFmpegCpp::reloadMediaInfo()
{
    if (!isOpen())
    {
        m_lastError = "file is not open";
        return false;
    }

    AVFormatContext* formatContext = static_cast<AVFormatContext*>(m_formatContext);
    m_lastError.clear();
    FFmpegCppMediaInfo mediaInfo;
    mediaInfo.filePath = m_filePath;
    if (formatContext->iformat != nullptr)
    {
        mediaInfo.formatName = formatContext->iformat->name != nullptr ? formatContext->iformat->name : std::string();
        mediaInfo.formatLongName = formatContext->iformat->long_name != nullptr ? formatContext->iformat->long_name : std::string();
    }
    mediaInfo.fileSizeBytes = ffmpegCppFileSize(m_filePath);
    if (formatContext->duration != AV_NOPTS_VALUE)
    {
        mediaInfo.durationMilliseconds = formatContext->duration * 1000 / AV_TIME_BASE;
    }
    mediaInfo.bitRate = formatContext->bit_rate;

    int mainVideoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int mainAudioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    mediaInfo.mainVideoStreamIndex = mainVideoStreamIndex >= 0 ? mainVideoStreamIndex : -1;
    mediaInfo.mainAudioStreamIndex = mainAudioStreamIndex >= 0 ? mainAudioStreamIndex : -1;
    mediaInfo.hasVideo = mediaInfo.mainVideoStreamIndex >= 0;
    mediaInfo.hasAudio = mediaInfo.mainAudioStreamIndex >= 0;

    for (unsigned int i = 0; i < formatContext->nb_streams; ++i)
    {
        mediaInfo.streams.push_back(ffmpegCppMakeStreamInfo(formatContext,
                                                            formatContext->streams[i],
                                                            mediaInfo.mainVideoStreamIndex,
                                                            mediaInfo.mainAudioStreamIndex));
    }

    m_mediaInfo = mediaInfo;
    return true;
}

FFmpegCppMediaInfo FFmpegCpp::mediaInfo() const
{
    return m_mediaInfo;
}

bool FFmpegCpp::getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const
{
    if (mediaInfo == nullptr)
    {
        return false;
    }
    if (!isOpen())
    {
        return false;
    }

    *mediaInfo = m_mediaInfo;
    return true;
}

bool FFmpegCpp::getMainVideoStreamInfo(FFmpegCppStreamInfo* streamInfo) const
{
    return m_mediaInfo.getMainVideoStreamInfo(streamInfo);
}

bool FFmpegCpp::getMainAudioStreamInfo(FFmpegCppStreamInfo* streamInfo) const
{
    return m_mediaInfo.getMainAudioStreamInfo(streamInfo);
}

bool FFmpegCpp::hasVideo() const
{
    return m_mediaInfo.hasVideo;
}

bool FFmpegCpp::hasAudio() const
{
    return m_mediaInfo.hasAudio;
}

int64_t FFmpegCpp::durationMilliseconds() const
{
    return m_mediaInfo.durationMilliseconds;
}

bool FFmpegCpp::extractFirstFrameToMemory(FFmpegCppImageData* imageData,
                                          const FFmpegCppFrameExtractOption& option)
{
    return extractFrameToMemory(0, imageData, option);
}

bool FFmpegCpp::extractFrameToMemory(int64_t timestampMilliseconds,
                                     FFmpegCppImageData* imageData,
                                     const FFmpegCppFrameExtractOption& option)
{
    if (imageData == nullptr)
    {
        m_lastError = "imageData is null";
        return false;
    }
    imageData->reset();
    if (!isOpen())
    {
        m_lastError = "file is not open";
        return false;
    }
    if (!hasVideo())
    {
        m_lastError = "video stream is not found";
        return false;
    }

    m_lastError.clear();
    if (timestampMilliseconds < 0)
    {
        timestampMilliseconds = 0;
    }

    m_cancelRequested = false;
    AVFormatContext* formatContext = static_cast<AVFormatContext*>(m_formatContext);
    int videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (videoStreamIndex < 0)
    {
        m_lastError = ffmpegCppAppendError("find video stream failed", videoStreamIndex);
        return false;
    }

    AVStream* videoStream = formatContext->streams[videoStreamIndex];
    const AVCodec* decoder = avcodec_find_decoder(videoStream->codecpar->codec_id);
    if (decoder == nullptr)
    {
        m_lastError = "video decoder is not enabled";
        return false;
    }

    AVCodecContext* decodeContext = avcodec_alloc_context3(decoder);
    if (decodeContext == nullptr)
    {
        m_lastError = "alloc video decoder context failed";
        return false;
    }

    int result = avcodec_parameters_to_context(decodeContext, videoStream->codecpar);
    if (result < 0)
    {
        avcodec_free_context(&decodeContext);
        m_lastError = ffmpegCppAppendError("copy video decoder parameter failed", result);
        return false;
    }

    result = avcodec_open2(decodeContext, decoder, nullptr);
    if (result < 0)
    {
        avcodec_free_context(&decodeContext);
        m_lastError = ffmpegCppAppendError("open video decoder failed", result);
        return false;
    }

    int64_t seekTimestamp = ffmpegCppFromMilliseconds(timestampMilliseconds, videoStream->time_base);
    result = av_seek_frame(formatContext, videoStreamIndex, seekTimestamp, AVSEEK_FLAG_BACKWARD);
    if (result < 0 && timestampMilliseconds > 0)
    {
        avcodec_free_context(&decodeContext);
        m_lastError = ffmpegCppAppendError("seek video failed", result);
        return false;
    }
    avcodec_flush_buffers(decodeContext);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    if (packet == nullptr || frame == nullptr)
    {
        av_packet_free(&packet);
        av_frame_free(&frame);
        avcodec_free_context(&decodeContext);
        m_lastError = "alloc decode packet or frame failed";
        return false;
    }

    bool foundFrame = false;
    std::string encodeError;
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    while (!foundFrame)
    {
        if (m_cancelRequested)
        {
            m_lastError = "extract frame cancelled";
            break;
        }
        if (ffmpegCppTimeoutReached(startTime, option.timeoutMilliseconds))
        {
            m_lastError = "extract frame timeout";
            break;
        }

        result = av_read_frame(formatContext, packet);
        if (result < 0)
        {
            result = avcodec_send_packet(decodeContext, nullptr);
            if (result < 0 && result != AVERROR_EOF)
            {
                m_lastError = ffmpegCppAppendError("flush decoder failed", result);
                break;
            }
        }
        else if (packet->stream_index == videoStreamIndex)
        {
            result = avcodec_send_packet(decodeContext, packet);
            av_packet_unref(packet);
            if (result < 0 && result != AVERROR(EAGAIN))
            {
                m_lastError = ffmpegCppAppendError("send packet to decoder failed", result);
                break;
            }
        }
        else
        {
            av_packet_unref(packet);
            continue;
        }

        while (true)
        {
            result = avcodec_receive_frame(decodeContext, frame);
            if (result == 0)
            {
                int64_t frameTimestamp = frame->best_effort_timestamp != AV_NOPTS_VALUE ? frame->best_effort_timestamp : frame->pts;
                int64_t frameMilliseconds = ffmpegCppToMilliseconds(frameTimestamp, videoStream->time_base);
                if (ffmpegCppEncodeFrameToImage(frame, decodeContext, frameMilliseconds, option, imageData, encodeError))
                {
                    if (imageData->timestampMilliseconds == 0)
                    {
                        imageData->timestampMilliseconds = timestampMilliseconds;
                    }
                    foundFrame = true;
                    break;
                }

                m_lastError = encodeError;
                break;
            }
            if (result == AVERROR(EAGAIN))
            {
                result = 0;
                break;
            }
            if (result == AVERROR_EOF)
            {
                m_lastError = "video frame is not found";
                break;
            }

            m_lastError = ffmpegCppAppendError("receive decoded frame failed", result);
            break;
        }

        if (result == AVERROR_EOF || (!foundFrame && !m_lastError.empty()))
        {
            break;
        }
        if (result < 0)
        {
            break;
        }
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&decodeContext);
    return foundFrame;
}

bool FFmpegCpp::extractFirstFrameToFile(const std::string& outputFilePath,
                                        const FFmpegCppFrameExtractOption& option,
                                        std::string* savedFilePath)
{
    return extractFrameToFile(0, outputFilePath, option, savedFilePath);
}

bool FFmpegCpp::extractFrameToFile(int64_t timestampMilliseconds,
                                   const std::string& outputFilePath,
                                   const FFmpegCppFrameExtractOption& option,
                                   std::string* savedFilePath)
{
    FFmpegCppImageData imageData;
    if (!extractFrameToMemory(timestampMilliseconds, &imageData, option))
    {
        return false;
    }

    std::string finalFilePath = outputFilePath;
    if (finalFilePath.empty())
    {
        finalFilePath = ffmpegCppMakeFrameFilePath(m_filePath, timestampMilliseconds, option.imageFormat);
    }

    std::ofstream stream(finalFilePath.c_str(), std::ios::binary);
    if (!stream.good())
    {
        m_lastError = "open output image failed";
        return false;
    }
    if (!imageData.data.empty())
    {
        stream.write(reinterpret_cast<const char*>(&imageData.data[0]), static_cast<std::streamsize>(imageData.data.size()));
    }
    if (!stream.good())
    {
        m_lastError = "write output image failed";
        return false;
    }

    if (savedFilePath != nullptr)
    {
        *savedFilePath = finalFilePath;
    }
    return true;
}

bool FFmpegCpp::transcode(const std::string& outputFilePath,
                          const FFmpegCppTranscodeOption& option,
                          FFmpegCppProgressCallback progressCallback,
                          void* userData)
{
    if (!isOpen())
    {
        m_lastError = "file is not open";
        return false;
    }
    if (outputFilePath.empty())
    {
        m_lastError = "outputFilePath is empty";
        return false;
    }
    if (!option.overwriteOutput && ffmpegCppFileExists(outputFilePath))
    {
        m_lastError = "output file already exists";
        return false;
    }
    if (option.videoCodec != FFmpegCppVideoCodecCopy
        || (!option.removeAudio && option.audioCodec != FFmpegCppAudioCodecCopy && option.audioCodec != FFmpegCppAudioCodecNone))
    {
        m_lastError = "current ffmpeg package supports stream-copy transcode only";
        return false;
    }

    m_lastError.clear();
    m_cancelRequested = false;
    AVFormatContext* inputContext = static_cast<AVFormatContext*>(m_formatContext);
    AVFormatContext* outputContext = nullptr;
    const char* formatName = option.outputContainer.empty() ? nullptr : option.outputContainer.c_str();
    int result = avformat_alloc_output_context2(&outputContext, nullptr, formatName, outputFilePath.c_str());
    if (result < 0 || outputContext == nullptr)
    {
        m_lastError = result < 0 ? ffmpegCppAppendError("alloc output context failed", result) : "alloc output context failed";
        return false;
    }

    std::vector<int> streamMapping(inputContext->nb_streams, -1);
    int outputStreamCount = 0;
    for (unsigned int i = 0; i < inputContext->nb_streams; ++i)
    {
        AVStream* inputStream = inputContext->streams[i];
        if (inputStream == nullptr || inputStream->codecpar == nullptr)
        {
            continue;
        }

        AVMediaType mediaType = inputStream->codecpar->codec_type;
        if (mediaType != AVMEDIA_TYPE_VIDEO && mediaType != AVMEDIA_TYPE_AUDIO && mediaType != AVMEDIA_TYPE_SUBTITLE)
        {
            continue;
        }
        if (mediaType == AVMEDIA_TYPE_AUDIO && (option.removeAudio || option.audioCodec == FFmpegCppAudioCodecNone))
        {
            continue;
        }

        AVStream* outputStream = avformat_new_stream(outputContext, nullptr);
        if (outputStream == nullptr)
        {
            avformat_free_context(outputContext);
            m_lastError = "create output stream failed";
            return false;
        }

        streamMapping[i] = outputStream->index;
        ++outputStreamCount;
        result = avcodec_parameters_copy(outputStream->codecpar, inputStream->codecpar);
        if (result < 0)
        {
            avformat_free_context(outputContext);
            m_lastError = ffmpegCppAppendError("copy output stream parameter failed", result);
            return false;
        }
        outputStream->codecpar->codec_tag = 0;
        outputStream->time_base = inputStream->time_base;
    }

    if (outputStreamCount == 0)
    {
        avformat_free_context(outputContext);
        m_lastError = "no stream can be copied";
        return false;
    }

    if ((outputContext->oformat->flags & AVFMT_NOFILE) == 0)
    {
        result = avio_open(&outputContext->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE);
        if (result < 0)
        {
            avformat_free_context(outputContext);
            m_lastError = ffmpegCppAppendError("open output file failed", result);
            return false;
        }
    }

    AVDictionary* muxOptions = nullptr;
    if (option.fastStart && ffmpegCppIsMovLikeOutput(outputContext->oformat))
    {
        av_dict_set(&muxOptions, "movflags", "faststart", 0);
    }
    result = avformat_write_header(outputContext, &muxOptions);
    av_dict_free(&muxOptions);
    if (result < 0)
    {
        if ((outputContext->oformat->flags & AVFMT_NOFILE) == 0)
        {
            avio_closep(&outputContext->pb);
        }
        avformat_free_context(outputContext);
        m_lastError = ffmpegCppAppendError("write output header failed", result);
        return false;
    }

    av_seek_frame(inputContext, -1, 0, AVSEEK_FLAG_BACKWARD);
    int64_t totalMilliseconds = durationMilliseconds();
    int64_t lastProgressMilliseconds = -1;
    bool finished = false;
    bool cancelled = false;
    AVPacket* packet = av_packet_alloc();
    if (packet == nullptr)
    {
        if ((outputContext->oformat->flags & AVFMT_NOFILE) == 0)
        {
            avio_closep(&outputContext->pb);
        }
        avformat_free_context(outputContext);
        m_lastError = "alloc transcode packet failed";
        return false;
    }

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    while (true)
    {
        if (m_cancelRequested)
        {
            cancelled = true;
            m_lastError = "transcode cancelled";
            break;
        }
        if (ffmpegCppTimeoutReached(startTime, option.timeoutMilliseconds))
        {
            cancelled = true;
            m_lastError = "transcode timeout";
            break;
        }

        result = av_read_frame(inputContext, packet);
        if (result < 0)
        {
            if (result == AVERROR_EOF)
            {
                finished = true;
                break;
            }
            m_lastError = ffmpegCppAppendError("read input packet failed", result);
            break;
        }

        int inputStreamIndex = packet->stream_index;
        if (inputStreamIndex < 0
            || inputStreamIndex >= static_cast<int>(streamMapping.size())
            || streamMapping[inputStreamIndex] < 0)
        {
            av_packet_unref(packet);
            continue;
        }

        AVStream* inputStream = inputContext->streams[inputStreamIndex];
        AVStream* outputStream = outputContext->streams[streamMapping[inputStreamIndex]];
        int64_t packetMilliseconds = ffmpegCppToMilliseconds(packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts, inputStream->time_base);
        packet->pts = av_rescale_q_rnd(packet->pts, inputStream->time_base, outputStream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet->dts = av_rescale_q_rnd(packet->dts, inputStream->time_base, outputStream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet->duration = av_rescale_q(packet->duration, inputStream->time_base, outputStream->time_base);
        packet->pos = -1;
        packet->stream_index = streamMapping[inputStreamIndex];

        result = av_interleaved_write_frame(outputContext, packet);
        av_packet_unref(packet);
        if (result < 0)
        {
            m_lastError = ffmpegCppAppendError("write output packet failed", result);
            break;
        }

        if (progressCallback != nullptr && packetMilliseconds != lastProgressMilliseconds)
        {
            lastProgressMilliseconds = packetMilliseconds;
            FFmpegCppProgress progress;
            progress.processedMilliseconds = packetMilliseconds;
            progress.totalMilliseconds = totalMilliseconds;
            progress.outputBytes = outputContext->pb != nullptr ? avio_tell(outputContext->pb) : 0;
            progress.percent = totalMilliseconds > 0 ? (std::min)(100.0, packetMilliseconds * 100.0 / totalMilliseconds) : 0.0;
            progress.message = "transcoding";
            if (!progressCallback(progress, userData))
            {
                cancelled = true;
                m_lastError = "transcode cancelled";
                break;
            }
        }
    }

    if (finished)
    {
        result = av_write_trailer(outputContext);
        if (result < 0)
        {
            finished = false;
            m_lastError = ffmpegCppAppendError("write output trailer failed", result);
        }
    }

    if (progressCallback != nullptr)
    {
        FFmpegCppProgress progress;
        progress.processedMilliseconds = finished ? totalMilliseconds : lastProgressMilliseconds;
        progress.totalMilliseconds = totalMilliseconds;
        progress.outputBytes = outputContext->pb != nullptr ? avio_tell(outputContext->pb) : ffmpegCppFileSize(outputFilePath);
        progress.percent = finished ? 100.0 : (totalMilliseconds > 0 ? (std::min)(100.0, progress.processedMilliseconds * 100.0 / totalMilliseconds) : 0.0);
        progress.finished = finished;
        progress.cancelled = cancelled;
        progress.message = finished ? "finished" : m_lastError;
        progressCallback(progress, userData);
    }

    av_packet_free(&packet);
    if ((outputContext->oformat->flags & AVFMT_NOFILE) == 0)
    {
        avio_closep(&outputContext->pb);
    }
    avformat_free_context(outputContext);
    return finished;
}

bool FFmpegCpp::cancelCurrentTask()
{
    m_cancelRequested = true;
    return true;
}

//static bool writeVideoPacket(AVFormatContext* formatContext, AVCodecContext* codecContext, AVStream* stream, AVPacket* packet)
//{
//    packet->stream_index = stream->index;
//    av_packet_rescale_ts(packet, codecContext->time_base, stream->time_base);
//    return av_interleaved_write_frame(formatContext, packet) == 0;
//}
//
//static bool receiveVideoPackets(AVFormatContext* formatContext, AVCodecContext* codecContext, AVStream* stream)
//{
//    AVPacket* packet = av_packet_alloc();
//    if (packet == nullptr)
//    {
//        return false;
//    }
//
//    bool result = true;
//    while (true)
//    {
//        int receiveResult = avcodec_receive_packet(codecContext, packet);
//        if (receiveResult == AVERROR(EAGAIN) || receiveResult == AVERROR_EOF)
//        {
//            break;
//        }
//        if (receiveResult < 0)
//        {
//            result = false;
//            break;
//        }
//        if (!writeVideoPacket(formatContext, codecContext, stream, packet))
//        {
//            result = false;
//            av_packet_unref(packet);
//            break;
//        }
//        av_packet_unref(packet);
//    }
//
//    av_packet_free(&packet);
//    return result;
//}
//
//static bool writeTestAvi(const std::string& filePath, int32_t width, int32_t height)
//{
//    AVFormatContext* formatContext = nullptr;
//    if (avformat_alloc_output_context2(&formatContext, nullptr, "avi", filePath.c_str()) < 0 || formatContext == nullptr)
//    {
//        return false;
//    }
//
//    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
//    if (codec == nullptr)
//    {
//        avformat_free_context(formatContext);
//        return false;
//    }
//
//    AVStream* stream = avformat_new_stream(formatContext, nullptr);
//    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
//    if (stream == nullptr || codecContext == nullptr)
//    {
//        avcodec_free_context(&codecContext);
//        avformat_free_context(formatContext);
//        return false;
//    }
//
//    codecContext->codec_id = AV_CODEC_ID_MJPEG;
//    codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
//    codecContext->width = width;
//    codecContext->height = height;
//    codecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
//    codecContext->time_base.num = 1;
//    codecContext->time_base.den = 25;
//    if ((formatContext->oformat->flags & AVFMT_GLOBALHEADER) != 0)
//    {
//        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//    }
//
//    bool result = false;
//    if (avcodec_open2(codecContext, codec, nullptr) >= 0
//        && avcodec_parameters_from_context(stream->codecpar, codecContext) >= 0)
//    {
//        stream->time_base = codecContext->time_base;
//        if ((formatContext->oformat->flags & AVFMT_NOFILE) != 0 || avio_open(&formatContext->pb, filePath.c_str(), AVIO_FLAG_WRITE) >= 0)
//        {
//            if (avformat_write_header(formatContext, nullptr) >= 0)
//            {
//                AVFrame* frame = av_frame_alloc();
//                if (frame != nullptr)
//                {
//                    frame->format = codecContext->pix_fmt;
//                    frame->width = width;
//                    frame->height = height;
//                    if (av_frame_get_buffer(frame, 32) >= 0)
//                    {
//                        result = true;
//                        for (int32_t frameIndex = 0; frameIndex < 5 && result; ++frameIndex)
//                        {
//                            if (av_frame_make_writable(frame) < 0)
//                            {
//                                result = false;
//                                break;
//                            }
//                            for (int32_t y = 0; y < height; ++y)
//                            {
//                                for (int32_t x = 0; x < width; ++x)
//                                {
//                                    frame->data[0][y * frame->linesize[0] + x] = static_cast<unsigned char>((x + y + frameIndex * 10) % 255);
//                                }
//                            }
//                            for (int32_t y = 0; y < height / 2; ++y)
//                            {
//                                for (int32_t x = 0; x < width / 2; ++x)
//                                {
//                                    frame->data[1][y * frame->linesize[1] + x] = static_cast<unsigned char>(90 + frameIndex * 10);
//                                    frame->data[2][y * frame->linesize[2] + x] = static_cast<unsigned char>(160 - frameIndex * 10);
//                                }
//                            }
//                            frame->pts = frameIndex;
//                            if (avcodec_send_frame(codecContext, frame) < 0)
//                            {
//                                result = false;
//                                break;
//                            }
//                            result = receiveVideoPackets(formatContext, codecContext, stream);
//                        }
//                        if (result && avcodec_send_frame(codecContext, nullptr) >= 0)
//                        {
//                            result = receiveVideoPackets(formatContext, codecContext, stream);
//                        }
//                    }
//                    av_frame_free(&frame);
//                }
//                result = av_write_trailer(formatContext) >= 0 && result;
//            }
//            if ((formatContext->oformat->flags & AVFMT_NOFILE) == 0)
//            {
//                avio_closep(&formatContext->pb);
//            }
//        }
//    }
//
//    avcodec_free_context(&codecContext);
//    avformat_free_context(formatContext);
//    return result;
//}
//
//static bool fileExistsAndNotEmpty(const std::string& filePath)
//{
//    std::ifstream stream(filePath.c_str(), std::ios::binary | std::ios::ate);
//    if (!stream.good())
//    {
//        return false;
//    }
//    return stream.tellg() > 0;
//}
//
//static bool progressCancelCallback(const FFmpegCppProgress&, void*)
//{
//    return false;
//}
//
//int main()
//{
//    av_log_set_level(AV_LOG_QUIET);
//
//    int totalCount = 0;
//    int failCount = 0;
//    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//    {
//        ++totalCount;
//        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//        if (!ok)
//        {
//            ++failCount;
//        }
//    };
//
//    const std::string inputAvi = "src/temp/ffmpegcpp_input.avi";
//    const std::string outputBmp = "src/temp/ffmpegcpp_frame.bmp";
//    const std::string copyOutput = "src/temp/ffmpegcpp_copy.avi";
//    remove(inputAvi.c_str());
//    remove(outputBmp.c_str());
//    remove(copyOutput.c_str());
//
//    FFmpegCpp emptyObject;
//    check(!emptyObject.open(""), "FFmpegCpp open empty path");
//    check(!emptyObject.isOpen(), "FFmpegCpp unopened state");
//    check(!emptyObject.getMediaInfo(nullptr), "FFmpegCpp get media info null");
//    check(!emptyObject.extractFirstFrameToMemory(nullptr), "FFmpegCpp extract null image data");
//    emptyObject.close();
//    emptyObject.close();
//    check(!emptyObject.isOpen(), "FFmpegCpp repeated close");
//
//    check(FFmpegCppHelper::isCommonVideoFileExtension("test.MP4"), "FFmpegCpp helper extension true");
//    check(!FFmpegCppHelper::isCommonVideoFileExtension("test.txt"), "FFmpegCpp helper extension false");
//    check(FFmpegCppHelper::guessVideoMimeTypeByFilePath("a.mkv") == "video/x-matroska", "FFmpegCpp helper mime");
//    check(FFmpegCppHelper::imageFormatToExtension(FFmpegCppImageFormatBmp) == "bmp", "FFmpegCpp helper image extension");
//    check(FFmpegCppHelper::imageFormatToMimeType(FFmpegCppImageFormatJpeg) == "image/jpeg", "FFmpegCpp helper image mime");
//    check(FFmpegCppHelper::videoCodecToEncoderName(FFmpegCppVideoCodecCopy) == "copy", "FFmpegCpp helper video codec copy");
//    check(FFmpegCppHelper::audioCodecToEncoderName(FFmpegCppAudioCodecNone).empty(), "FFmpegCpp helper audio none");
//    check(FFmpegCppHelper::formatMilliseconds(3723004) == "01:02:03.004", "FFmpegCpp helper time text");
//
//    std::string errorText;
//    FFmpegCppMediaInfo mediaInfo;
//    check(!FFmpegCppHelper::probeMediaInfo("", &mediaInfo, &errorText) && !errorText.empty(), "FFmpegCpp helper probe invalid");
//    check(!FFmpegCppHelper::probeMediaInfo(inputAvi, nullptr, &errorText) && !errorText.empty(), "FFmpegCpp helper probe null");
//
//    check(writeTestAvi(inputAvi, 64, 48), "FFmpegCpp write test avi");
//
//    FFmpegCpp ffmpegCpp;
//    check(ffmpegCpp.open(inputAvi), "FFmpegCpp open avi");
//    check(ffmpegCpp.isOpen() && ffmpegCpp.filePath() == inputAvi, "FFmpegCpp open state");
//    check(ffmpegCpp.reloadMediaInfo(), "FFmpegCpp reload media info");
//    check(ffmpegCpp.getMediaInfo(&mediaInfo), "FFmpegCpp get media info");
//    check(mediaInfo.fileSizeBytes > 0 && mediaInfo.hasVideo && !mediaInfo.hasAudio, "FFmpegCpp media info values");
//    FFmpegCppStreamInfo videoStream;
//    check(ffmpegCpp.getMainVideoStreamInfo(&videoStream) && videoStream.width == 64 && videoStream.height == 48, "FFmpegCpp main video stream");
//    check(!ffmpegCpp.getMainAudioStreamInfo(&videoStream), "FFmpegCpp missing audio stream");
//
//    FFmpegCppFrameExtractOption extractOption;
//    extractOption.maxWidth = 32;
//    extractOption.maxHeight = 32;
//    extractOption.imageFormat = FFmpegCppImageFormatJpeg;
//    FFmpegCppImageData imageData;
//    bool extractJpegResult = ffmpegCpp.extractFirstFrameToMemory(&imageData, extractOption);
//    if (!extractJpegResult)
//    {
//        std::cout << "extract jpeg error: " << ffmpegCpp.lastError() << std::endl;
//    }
//    check(extractJpegResult, "FFmpegCpp extract first frame jpeg");
//    check(!imageData.empty() && imageData.width <= 32 && imageData.height <= 32 && imageData.mimeType == "image/jpeg", "FFmpegCpp jpeg image data");
//
//    extractOption.imageFormat = FFmpegCppImageFormatBmp;
//    std::string savedFilePath;
//    bool extractBmpFileResult = ffmpegCpp.extractFrameToFile(0, outputBmp, extractOption, &savedFilePath);
//    if (!extractBmpFileResult)
//    {
//        std::cout << "extract bmp error: " << ffmpegCpp.lastError() << std::endl;
//    }
//    check(extractBmpFileResult, "FFmpegCpp extract frame file bmp");
//    check(savedFilePath == outputBmp && fileExistsAndNotEmpty(outputBmp), "FFmpegCpp output bmp exists");
//
//    extractOption.imageFormat = FFmpegCppImageFormatPng;
//    check(!ffmpegCpp.extractFirstFrameToMemory(&imageData, extractOption) && !ffmpegCpp.lastError().empty(), "FFmpegCpp unsupported png branch");
//
//    FFmpegCppTranscodeOption transcodeOption;
//    check(!ffmpegCpp.transcode("", transcodeOption), "FFmpegCpp transcode empty output");
//    transcodeOption.videoCodec = FFmpegCppVideoCodecH264;
//    transcodeOption.audioCodec = FFmpegCppAudioCodecAac;
//    check(!ffmpegCpp.transcode(copyOutput, transcodeOption), "FFmpegCpp unsupported encode transcode");
//    transcodeOption.videoCodec = FFmpegCppVideoCodecCopy;
//    transcodeOption.audioCodec = FFmpegCppAudioCodecNone;
//    transcodeOption.removeAudio = true;
//    transcodeOption.outputContainer = "avi";
//    transcodeOption.overwriteOutput = true;
//    bool copyResult = ffmpegCpp.transcode(copyOutput, transcodeOption, progressCancelCallback, nullptr);
//    check(!copyResult || fileExistsAndNotEmpty(copyOutput), "FFmpegCpp copy transcode branch");
//    check(ffmpegCpp.cancelCurrentTask(), "FFmpegCpp cancel flag");
//    ffmpegCpp.close();
//    check(!ffmpegCpp.isOpen(), "FFmpegCpp close after work");
//
//    FFmpegCppImageData helperImage;
//    extractOption.imageFormat = FFmpegCppImageFormatBmp;
//    bool helperExtractResult = FFmpegCppHelper::extractFirstFrameToMemory(inputAvi, &helperImage, extractOption, &errorText);
//    if (!helperExtractResult)
//    {
//        std::cout << "helper extract error: " << errorText << std::endl;
//    }
//    check(helperExtractResult, "FFmpegCpp helper extract memory");
//    check(!helperImage.empty(), "FFmpegCpp helper image data");
//
//    remove(inputAvi.c_str());
//    remove(outputBmp.c_str());
//    remove(copyOutput.c_str());
//
//    std::cout << "FFmpegCpp runtime test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
//    return failCount == 0 ? 0 : 1;
//}