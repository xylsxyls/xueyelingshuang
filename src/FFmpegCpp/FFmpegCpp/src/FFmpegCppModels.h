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
public:
    // 分子
    int32_t numerator;
    // 分母
    int32_t denominator;

public:
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
public:
    // 流下标
    int32_t streamIndex;
    // 媒体类型
    FFmpegCppMediaType mediaType;
    // 编码名称
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string codecName;
    // 编码完整名称
    std::string codecLongName;
    // 编码profile
    std::string profile;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 视频宽度
    int32_t width;
    // 视频高度
    int32_t height;
    // 视频旋转角度
    int32_t rotationDegree;
    // 流时长，单位毫秒
    int64_t durationMilliseconds;
    // 流码率，单位bit/s
    int64_t bitRate;
    // 视频帧率
    FFmpegCppRational frameRate;
    // 流时间基
    FFmpegCppRational timeBase;
    // 音频采样率
    int32_t sampleRate;
    // 音频声道数
    int32_t channels;
    // 流语言标记
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string language;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
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
public:
    // 媒体文件路径
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string filePath;
    // 容器格式名称
    std::string formatName;
    // 容器格式完整名称
    std::string formatLongName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 文件大小，单位字节
    int64_t fileSizeBytes;
    // 媒体总时长，单位毫秒
    int64_t durationMilliseconds;
    // 媒体总码率，单位bit/s
    int64_t bitRate;
    // 是否包含视频流
    bool hasVideo;
    // 是否包含音频流
    bool hasAudio;
    // 主视频流下标
    int32_t mainVideoStreamIndex;
    // 主音频流下标
    int32_t mainAudioStreamIndex;
    // 全部流信息
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<FFmpegCppStreamInfo> streams;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
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
public:
    // 图片格式
    FFmpegCppImageFormat imageFormat;
    // 图片mimeType
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string mimeType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 图片宽度
    int32_t width;
    // 图片高度
    int32_t height;
    // 截图时间点，单位毫秒
    int64_t timestampMilliseconds;
    // 图片二进制数据
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<unsigned char> data;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
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
public:
    // 截图时间点，单位毫秒
    int64_t timestampMilliseconds;
    // 输出图片最大宽度，小于等于0表示不限制
    int32_t maxWidth;
    // 输出图片最大高度，小于等于0表示不限制
    int32_t maxHeight;
    // 是否保持宽高比
    bool keepAspectRatio;
    // 是否按旋转元信息修正方向
    bool applyRotation;
    // 输出图片格式
    FFmpegCppImageFormat imageFormat;
    // JPEG质量，范围1-100
    int32_t jpegQuality;
    // 截图超时时间，单位毫秒，小于等于0表示不限制
    int32_t timeoutMilliseconds;
    // 是否允许硬件解码
    bool allowHardwareDecode;

public:
    /** 构造函数，默认截取0毫秒位置的jpeg图片，不强制缩放
    */
    FFmpegCppFrameExtractOption();
};

/** 视频转码选项
*/
struct FFmpegCppAPI FFmpegCppTranscodeOption
{
public:
    // 输出容器名称，为空时根据输出文件后缀推断
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string outputContainer;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 目标视频编码
    FFmpegCppVideoCodec videoCodec;
    // 目标音频编码
    FFmpegCppAudioCodec audioCodec;
    // 输出视频最大宽度，小于等于0表示不限制
    int32_t maxWidth;
    // 输出视频最大高度，小于等于0表示不限制
    int32_t maxHeight;
    // 是否保持宽高比
    bool keepAspectRatio;
    // 视频码率，单位kbps，小于等于0表示使用编码器默认值
    int32_t videoBitRateKbps;
    // 音频码率，单位kbps
    int32_t audioBitRateKbps;
    // CRF质量参数
    int32_t crf;
    // 编码预设
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string preset;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // MP4/MOV类容器是否启用faststart
    bool fastStart;
    // 输出文件存在时是否覆盖
    bool overwriteOutput;
    // 是否移除音频
    bool removeAudio;
    // 是否允许硬件编码
    bool allowHardwareEncode;
    // 转码超时时间，单位毫秒，小于等于0表示不限制
    int32_t timeoutMilliseconds;

public:
    /** 构造函数，默认输出微信小程序兼容性较好的mp4/h264/aac组合
    */
    FFmpegCppTranscodeOption();
};

/** 长任务进度信息
*/
struct FFmpegCppAPI FFmpegCppProgress
{
public:
    // 已处理时长，单位毫秒
    int64_t processedMilliseconds;
    // 总时长，单位毫秒
    int64_t totalMilliseconds;
    // 已写入输出字节数
    int64_t outputBytes;
    // 当前百分比，范围0-100
    double percent;
    // 是否已经完成
    bool finished;
    // 是否已经取消
    bool cancelled;
    // 进度说明文本
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string message;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
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