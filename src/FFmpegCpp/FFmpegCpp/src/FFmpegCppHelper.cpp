#include "FFmpegCppHelper.h"
#include "FFmpegCpp.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
}

static std::string ffmpegCppLowerString(const std::string& value)
{
    std::string result = value;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char item) -> char
    {
        return static_cast<char>(std::tolower(item));
    });
    return result;
}

static std::string ffmpegCppFileExtension(const std::string& filePath)
{
    size_t slashPos = filePath.find_last_of("/\\");
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos)
    {
        return std::string();
    }
    if (slashPos != std::string::npos && dotPos < slashPos)
    {
        return std::string();
    }
    return ffmpegCppLowerString(filePath.substr(dotPos + 1));
}

static void ffmpegCppSetErrorText(std::string* errorText, const std::string& value)
{
    if (errorText != nullptr)
    {
        *errorText = value;
    }
}

static AVCodecID ffmpegCppVideoCodecIdFromPublicEnum(FFmpegCppVideoCodec videoCodec)
{
    if (videoCodec == FFmpegCppVideoCodecH264)
    {
        return AV_CODEC_ID_H264;
    }
    if (videoCodec == FFmpegCppVideoCodecH265)
    {
        return AV_CODEC_ID_HEVC;
    }
    if (videoCodec == FFmpegCppVideoCodecVp9)
    {
        return AV_CODEC_ID_VP9;
    }
    if (videoCodec == FFmpegCppVideoCodecAv1)
    {
        return AV_CODEC_ID_AV1;
    }
    return AV_CODEC_ID_NONE;
}

static AVCodecID ffmpegCppAudioCodecIdFromPublicEnum(FFmpegCppAudioCodec audioCodec)
{
    if (audioCodec == FFmpegCppAudioCodecAac)
    {
        return AV_CODEC_ID_AAC;
    }
    if (audioCodec == FFmpegCppAudioCodecMp3)
    {
        return AV_CODEC_ID_MP3;
    }
    if (audioCodec == FFmpegCppAudioCodecOpus)
    {
        return AV_CODEC_ID_OPUS;
    }
    return AV_CODEC_ID_NONE;
}

static bool ffmpegCppIsEncoderAvailable(const std::string& encoderName, AVCodecID codecId)
{
    if (!encoderName.empty() && encoderName != "copy" && avcodec_find_encoder_by_name(encoderName.c_str()) != nullptr)
    {
        return true;
    }
    return codecId != AV_CODEC_ID_NONE && avcodec_find_encoder(codecId) != nullptr;
}

bool FFmpegCppHelper::probeMediaInfo(const std::string& filePath,
                                     FFmpegCppMediaInfo* mediaInfo,
                                     std::string* errorText)
{
    if (mediaInfo == nullptr)
    {
        ffmpegCppSetErrorText(errorText, "mediaInfo is null");
        return false;
    }

    FFmpegCpp ffmpegCpp;
    if (!ffmpegCpp.open(filePath))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }

    if (!ffmpegCpp.getMediaInfo(mediaInfo))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }
    return true;
}

bool FFmpegCppHelper::extractFirstFrameToMemory(const std::string& filePath,
                                                FFmpegCppImageData* imageData,
                                                const FFmpegCppFrameExtractOption& option,
                                                std::string* errorText)
{
    return extractFrameToMemory(filePath, 0, imageData, option, errorText);
}

bool FFmpegCppHelper::extractFrameToMemory(const std::string& filePath,
                                           int64_t timestampMilliseconds,
                                           FFmpegCppImageData* imageData,
                                           const FFmpegCppFrameExtractOption& option,
                                           std::string* errorText)
{
    FFmpegCpp ffmpegCpp;
    if (!ffmpegCpp.open(filePath))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }

    if (!ffmpegCpp.extractFrameToMemory(timestampMilliseconds, imageData, option))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }
    return true;
}

bool FFmpegCppHelper::extractFirstFrameToFile(const std::string& filePath,
                                              const std::string& outputFilePath,
                                              const FFmpegCppFrameExtractOption& option,
                                              std::string* savedFilePath,
                                              std::string* errorText)
{
    return extractFrameToFile(filePath, 0, outputFilePath, option, savedFilePath, errorText);
}

bool FFmpegCppHelper::extractFrameToFile(const std::string& filePath,
                                         int64_t timestampMilliseconds,
                                         const std::string& outputFilePath,
                                         const FFmpegCppFrameExtractOption& option,
                                         std::string* savedFilePath,
                                         std::string* errorText)
{
    FFmpegCpp ffmpegCpp;
    if (!ffmpegCpp.open(filePath))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }

    if (!ffmpegCpp.extractFrameToFile(timestampMilliseconds, outputFilePath, option, savedFilePath))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }
    return true;
}

bool FFmpegCppHelper::transcodeVideo(const std::string& inputFilePath,
                                     const std::string& outputFilePath,
                                     const FFmpegCppTranscodeOption& option,
                                     FFmpegCppProgressCallback progressCallback,
                                     void* userData,
                                     std::string* errorText)
{
    FFmpegCpp ffmpegCpp;
    if (!ffmpegCpp.open(inputFilePath))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }

    if (!ffmpegCpp.transcode(outputFilePath, option, progressCallback, userData))
    {
        ffmpegCppSetErrorText(errorText, ffmpegCpp.lastError());
        return false;
    }
    return true;
}

bool FFmpegCppHelper::isCommonVideoFileExtension(const std::string& filePath)
{
    std::string extension = ffmpegCppFileExtension(filePath);
    return extension == "mp4"
        || extension == "mov"
        || extension == "mkv"
        || extension == "avi"
        || extension == "flv"
        || extension == "webm"
        || extension == "m4v"
        || extension == "3gp"
        || extension == "ts"
        || extension == "mpg"
        || extension == "mpeg";
}

std::string FFmpegCppHelper::guessVideoMimeTypeByFilePath(const std::string& filePath)
{
    std::string extension = ffmpegCppFileExtension(filePath);
    if (extension == "mp4" || extension == "m4v")
    {
        return "video/mp4";
    }
    if (extension == "mov")
    {
        return "video/quicktime";
    }
    if (extension == "mkv")
    {
        return "video/x-matroska";
    }
    if (extension == "avi")
    {
        return "video/x-msvideo";
    }
    if (extension == "flv")
    {
        return "video/x-flv";
    }
    if (extension == "webm")
    {
        return "video/webm";
    }
    if (extension == "3gp")
    {
        return "video/3gpp";
    }
    if (extension == "ts")
    {
        return "video/mp2t";
    }
    if (extension == "mpg" || extension == "mpeg")
    {
        return "video/mpeg";
    }
    return "application/octet-stream";
}

std::string FFmpegCppHelper::imageFormatToExtension(FFmpegCppImageFormat imageFormat)
{
    if (imageFormat == FFmpegCppImageFormatPng)
    {
        return "png";
    }
    if (imageFormat == FFmpegCppImageFormatBmp)
    {
        return "bmp";
    }
    return "jpg";
}

std::string FFmpegCppHelper::imageFormatToMimeType(FFmpegCppImageFormat imageFormat)
{
    if (imageFormat == FFmpegCppImageFormatPng)
    {
        return "image/png";
    }
    if (imageFormat == FFmpegCppImageFormatBmp)
    {
        return "image/bmp";
    }
    return "image/jpeg";
}

std::string FFmpegCppHelper::videoCodecToEncoderName(FFmpegCppVideoCodec videoCodec)
{
    if (videoCodec == FFmpegCppVideoCodecCopy)
    {
        return "copy";
    }
    if (videoCodec == FFmpegCppVideoCodecH264)
    {
        return "libx264";
    }
    if (videoCodec == FFmpegCppVideoCodecH265)
    {
        return "libx265";
    }
    if (videoCodec == FFmpegCppVideoCodecVp9)
    {
        return "libvpx-vp9";
    }
    if (videoCodec == FFmpegCppVideoCodecAv1)
    {
        return "libaom-av1";
    }
    return std::string();
}

bool FFmpegCppHelper::isVideoEncoderAvailable(FFmpegCppVideoCodec videoCodec)
{
    if (videoCodec == FFmpegCppVideoCodecCopy)
    {
        return true;
    }
    return ffmpegCppIsEncoderAvailable(videoCodecToEncoderName(videoCodec),
                                       ffmpegCppVideoCodecIdFromPublicEnum(videoCodec));
}

std::string FFmpegCppHelper::audioCodecToEncoderName(FFmpegCppAudioCodec audioCodec)
{
    if (audioCodec == FFmpegCppAudioCodecCopy)
    {
        return "copy";
    }
    if (audioCodec == FFmpegCppAudioCodecAac)
    {
        return "aac";
    }
    if (audioCodec == FFmpegCppAudioCodecMp3)
    {
        return "libmp3lame";
    }
    if (audioCodec == FFmpegCppAudioCodecOpus)
    {
        return "libopus";
    }
    return std::string();
}

bool FFmpegCppHelper::isAudioEncoderAvailable(FFmpegCppAudioCodec audioCodec)
{
    if (audioCodec == FFmpegCppAudioCodecCopy || audioCodec == FFmpegCppAudioCodecNone)
    {
        return true;
    }
    return ffmpegCppIsEncoderAvailable(audioCodecToEncoderName(audioCodec),
                                       ffmpegCppAudioCodecIdFromPublicEnum(audioCodec));
}

std::string FFmpegCppHelper::formatMilliseconds(int64_t milliseconds)
{
    if (milliseconds < 0)
    {
        milliseconds = 0;
    }

    int64_t hours = milliseconds / 3600000;
    milliseconds %= 3600000;
    int64_t minutes = milliseconds / 60000;
    milliseconds %= 60000;
    int64_t seconds = milliseconds / 1000;
    milliseconds %= 1000;

    std::ostringstream stream;
    stream << std::setfill('0')
           << std::setw(2) << hours << ":"
           << std::setw(2) << minutes << ":"
           << std::setw(2) << seconds << "."
           << std::setw(3) << milliseconds;
    return stream.str();
}