#pragma once
#include "FFmpegCppMacro.h"
#include <stdint.h>
#include <string>
#include <vector>

/** 媒体流类型
*/
enum FFmpegCppMediaType
{
    FFmpegCppMediaTypeUnknown = 0,
    FFmpegCppMediaTypeVideo = 1,
    FFmpegCppMediaTypeAudio = 2,
    FFmpegCppMediaTypeSubtitle = 3
};

/** 截图输出图片格式
*/
enum FFmpegCppImageFormat
{
    FFmpegCppImageFormatJpeg = 0,
    FFmpegCppImageFormatPng = 1,
    FFmpegCppImageFormatBmp = 2
};

/** 转码目标视频编码类型
*/
enum FFmpegCppVideoCodec
{
    FFmpegCppVideoCodecCopy = 0,
    FFmpegCppVideoCodecH264 = 1,
    FFmpegCppVideoCodecH265 = 2,
    FFmpegCppVideoCodecVp9 = 3,
    FFmpegCppVideoCodecAv1 = 4
};

/** 转码目标音频编码类型
*/
enum FFmpegCppAudioCodec
{
    FFmpegCppAudioCodecCopy = 0,
    FFmpegCppAudioCodecAac = 1,
    FFmpegCppAudioCodecMp3 = 2,
    FFmpegCppAudioCodecOpus = 3,
    FFmpegCppAudioCodecNone = 4
};

/** 有理数结构，用于帧率、timeBase等ffmpeg常见分数值
*/
struct FFmpegCppAPI FFmpegCppRational
{
    int32_t numerator;
    int32_t denominator;

    /** 构造函数，默认表示0/1
    */
    FFmpegCppRational();

    /** 按分子分母构造
    @param [in] numeratorValue 分子
    @param [in] denominatorValue 分母；传0时仍保存为0，由调用方决定是否有效
    */
    FFmpegCppRational(int32_t numeratorValue, int32_t denominatorValue);

    /** 转换成double值
    @param [in] defaultValue 分母为0时返回的默认值
    @return 返回numerator/denominator的double结果
    */
    double toDouble(double defaultValue = 0.0) const;
};

/** 媒体流信息，描述视频、音频或字幕中的单个stream
*/
struct FFmpegCppAPI FFmpegCppStreamInfo
{
    int32_t streamIndex;
    FFmpegCppMediaType mediaType;
    std::string codecName;
    std::string codecLongName;
    std::string profile;
    int32_t width;
    int32_t height;
    int32_t rotationDegree;
    int64_t durationMilliseconds;
    int64_t bitRate;
    FFmpegCppRational frameRate;
    FFmpegCppRational timeBase;
    int32_t sampleRate;
    int32_t channels;
    std::string language;

    /** 构造函数，初始化为无效空流
    */
    FFmpegCppStreamInfo();

    /** 判断当前流是否是视频流
    @return 返回true表示mediaType为FFmpegCppMediaTypeVideo
    */
    bool isVideo() const;

    /** 判断当前流是否是音频流
    @return 返回true表示mediaType为FFmpegCppMediaTypeAudio
    */
    bool isAudio() const;
};

/** 媒体文件整体信息
*/
struct FFmpegCppAPI FFmpegCppMediaInfo
{
    std::string filePath;
    std::string formatName;
    std::string formatLongName;
    int64_t fileSizeBytes;
    int64_t durationMilliseconds;
    int64_t bitRate;
    bool hasVideo;
    bool hasAudio;
    int32_t mainVideoStreamIndex;
    int32_t mainAudioStreamIndex;
    std::vector<FFmpegCppStreamInfo> streams;

    /** 构造函数，初始化为空媒体信息
    */
    FFmpegCppMediaInfo();

    /** 清空所有字段并恢复默认值
    */
    void reset();

    /** 获取主视频流信息
    @param [out] streamInfo 输出主视频流信息，不能为空
    @return 返回true表示存在主视频流且streamInfo已写入
    */
    bool getMainVideoStreamInfo(FFmpegCppStreamInfo* streamInfo) const;

    /** 获取主音频流信息
    @param [out] streamInfo 输出主音频流信息，不能为空
    @return 返回true表示存在主音频流且streamInfo已写入
    */
    bool getMainAudioStreamInfo(FFmpegCppStreamInfo* streamInfo) const;
};

/** 截图输出图片数据
*/
struct FFmpegCppAPI FFmpegCppImageData
{
    FFmpegCppImageFormat imageFormat;
    std::string mimeType;
    int32_t width;
    int32_t height;
    int64_t timestampMilliseconds;
    std::vector<unsigned char> data;

    /** 构造函数，初始化为空图片
    */
    FFmpegCppImageData();

    /** 清空图片内容和元信息
    */
    void reset();

    /** 判断图片数据是否为空
    @return 返回true表示data为空
    */
    bool empty() const;
};

/** 视频截图选项
*/
struct FFmpegCppAPI FFmpegCppFrameExtractOption
{
    int64_t timestampMilliseconds;
    int32_t maxWidth;
    int32_t maxHeight;
    bool keepAspectRatio;
    bool applyRotation;
    FFmpegCppImageFormat imageFormat;
    int32_t jpegQuality;
    int32_t timeoutMilliseconds;
    bool allowHardwareDecode;

    /** 构造函数，默认截取0毫秒位置的jpeg图片，不强制缩放
    */
    FFmpegCppFrameExtractOption();
};

/** 视频转码选项
*/
struct FFmpegCppAPI FFmpegCppTranscodeOption
{
    std::string outputContainer;
    FFmpegCppVideoCodec videoCodec;
    FFmpegCppAudioCodec audioCodec;
    int32_t maxWidth;
    int32_t maxHeight;
    bool keepAspectRatio;
    int32_t videoBitRateKbps;
    int32_t audioBitRateKbps;
    int32_t crf;
    std::string preset;
    bool fastStart;
    bool overwriteOutput;
    bool removeAudio;
    bool allowHardwareEncode;
    int32_t timeoutMilliseconds;

    /** 构造函数，默认输出微信小程序兼容性较好的mp4/h264/aac组合
    */
    FFmpegCppTranscodeOption();
};

/** 长任务进度信息
*/
struct FFmpegCppAPI FFmpegCppProgress
{
    int64_t processedMilliseconds;
    int64_t totalMilliseconds;
    int64_t outputBytes;
    double percent;
    bool finished;
    bool cancelled;
    std::string message;

    /** 构造函数，初始化为0进度
    */
    FFmpegCppProgress();
};

/** 进度回调函数类型
@param [in] progress 当前进度信息
@param [in,out] userData 调用方传入的自定义指针，库内部不读取也不释放
@return 返回true表示继续任务，返回false表示请求取消任务
*/
typedef bool (*FFmpegCppProgressCallback)(const FFmpegCppProgress& progress, void* userData);

#include "FFmpegCppModels.inl"