#include "FFmpegCppHelper.h"
#include "FFmpegCpp.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <cstring>

#ifdef _MSC_VER
#pragma warning(disable:4819)
#endif

extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libswscale/swscale.h>
}

bool FFmpegCppHelper::fitBgraFrame(const std::vector<unsigned char>& pixels, int32_t width, int32_t height,
    int32_t stride, int32_t outputWidth, int32_t outputHeight, std::vector<unsigned char>* output)
{
    if (!output || width < 1 || height < 1 || outputWidth < 1 || outputHeight < 1 ||
        width > 32768 || height > 32768 || outputWidth > 32768 || outputHeight > 32768 ||
        stride < width * 4 || static_cast<uint64_t>(stride) * height > pixels.size() ||
        static_cast<uint64_t>(outputWidth) * outputHeight * 4 > 512ULL * 1024 * 1024)
    {
        return false;
    }
    try
    {
        output->assign(static_cast<size_t>(outputWidth) * outputHeight * 4, 0);
        int32_t fitWidth = width, fitHeight = height;
        if (width > outputWidth || height > outputHeight || outputWidth - width > 1 || outputHeight - height > 1)
        {
            const double factor = (std::min)(static_cast<double>(outputWidth) / width, static_cast<double>(outputHeight) / height);
            fitWidth = (std::max)(1, static_cast<int32_t>(width * factor));
            fitHeight = (std::max)(1, static_cast<int32_t>(height * factor));
        }
        unsigned char* destination = output->data() + ((outputHeight - fitHeight) / 2 * outputWidth + (outputWidth - fitWidth) / 2) * 4;
        if (width == fitWidth && height == fitHeight)
        {
            for (int32_t y = 0; y < height; ++y)
            {
                std::memcpy(destination + y * outputWidth * 4, pixels.data() + static_cast<size_t>(y) * stride, width * 4);
            }
            return true;
        }
        SwsContext* scaler = sws_getContext(width, height, AV_PIX_FMT_BGRA, fitWidth, fitHeight,
            AV_PIX_FMT_BGRA, SWS_BICUBIC, nullptr, nullptr, nullptr);
        if (!scaler) return false;
        const unsigned char* inputs[4] = {pixels.data(), nullptr, nullptr, nullptr};
        unsigned char* outputs[4] = {destination, nullptr, nullptr, nullptr};
        const int inputStride[4] = {stride, 0, 0, 0}, outputStride[4] = {outputWidth * 4, 0, 0, 0};
        const int lines = sws_scale(scaler, inputs, inputStride, 0, height, outputs, outputStride);
        sws_freeContext(scaler);
        return lines == fitHeight;
    }
    catch (...)
    {
        return false;
    }
}

/** 把文本中的字符按当前C字符分类规则转换为小写
@param [in] value 输入文本，调用期间借用
@return 小写文本副本
*/
static std::string ffmpegCppLowerString(const std::string& value)
{
    std::string result = value;

    /** 将单个无符号字符转换为小写，避免负char传入tolower
    @param [in] item 输入字符的无符号值
    @return 小写字符
    */
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char item) -> char
    {
        return static_cast<char>(std::tolower(item));
    });
    return result;
}

/** 取出文件名最后一个点之后的扩展名并转为小写
@param [in] filePath 文件路径，调用期间借用
@return 不含点的扩展名，无有效扩展名返回空字符串
*/
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

/** 在调用方提供输出地址时复制错误文本
@param [out] errorText 可选的错误文本输出地址，为空时不写入
@param [in] value 输入文本，调用期间借用
*/
static void ffmpegCppSetErrorText(std::string* errorText, const std::string& value)
{
    if (errorText != nullptr)
    {
        *errorText = value;
    }
}

/** 把公共视频编码枚举转换为FFmpeg编码ID
@param [in] videoCodec 请求的视频编码类型
@return 对应编码ID，Copy及未识别值返回NONE
*/
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

/** 把公共音频编码枚举转换为FFmpeg编码ID
@param [in] audioCodec 请求的音频编码类型
@return 对应编码ID，Copy、None及未识别值返回NONE
*/
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

/** 依次按名称及编码ID检查当前运行库是否提供编码器
@param [in] encoderName 候选编码器名称，copy或空文本不按名称查找
@param [in] codecId 名称查找失败后使用的编码ID，NONE表示不再查找
@return true表示至少一种查找方式找到编码器
*/
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