#pragma once
#include "FFmpegCppMacro.h"
#include "FFmpegCppModels.h"

/** FFmpeg常用功能辅助类，适合一次性静态调用，不需要调用方长期持有视频对象
*/
class FFmpegCppAPI FFmpegCppHelper
{
public:
    /** 读取媒体文件基础信息
    @param [in] filePath 媒体文件完整路径
    @param [out] mediaInfo 输出媒体信息，不能为空
    @param [out] errorText 可选输出错误文本，传nullptr表示不需要
    @return 返回true表示读取成功
    */
    static bool probeMediaInfo(const std::string& filePath,
                               FFmpegCppMediaInfo* mediaInfo,
                               std::string* errorText = nullptr);

    /** 截取媒体文件首帧图片到内存
    @param [in] filePath 视频文件完整路径
    @param [out] imageData 输出编码后的图片数据，不能为空
    @param [in] option 截图选项，timestampMilliseconds会被当作0处理
    @param [out] errorText 可选输出错误文本，传nullptr表示不需要
    @return 返回true表示截图成功
    */
    static bool extractFirstFrameToMemory(const std::string& filePath,
                                          FFmpegCppImageData* imageData,
                                          const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption(),
                                          std::string* errorText = nullptr);

    /** 截取媒体文件指定时间点的一帧图片到内存
    @param [in] filePath 视频文件完整路径
    @param [in] timestampMilliseconds 截图时间点，单位毫秒；小于0时按0处理
    @param [out] imageData 输出编码后的图片数据，不能为空
    @param [in] option 截图选项；option.timestampMilliseconds会被timestampMilliseconds参数覆盖
    @param [out] errorText 可选输出错误文本，传nullptr表示不需要
    @return 返回true表示截图成功
    */
    static bool extractFrameToMemory(const std::string& filePath,
                                     int64_t timestampMilliseconds,
                                     FFmpegCppImageData* imageData,
                                     const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption(),
                                     std::string* errorText = nullptr);

    /** 截取媒体文件首帧图片到本地文件
    @param [in] filePath 视频文件完整路径
    @param [in] outputFilePath 输出图片完整路径；为空时实现层可自动生成路径
    @param [in] option 截图选项，timestampMilliseconds会被当作0处理
    @param [out] savedFilePath 可选输出最终保存路径，传nullptr表示不需要
    @param [out] errorText 可选输出错误文本，传nullptr表示不需要
    @return 返回true表示截图保存成功
    */
    static bool extractFirstFrameToFile(const std::string& filePath,
                                        const std::string& outputFilePath,
                                        const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption(),
                                        std::string* savedFilePath = nullptr,
                                        std::string* errorText = nullptr);

    /** 截取媒体文件指定时间点的一帧图片到本地文件
    @param [in] filePath 视频文件完整路径
    @param [in] timestampMilliseconds 截图时间点，单位毫秒；小于0时按0处理
    @param [in] outputFilePath 输出图片完整路径；为空时实现层可自动生成路径
    @param [in] option 截图选项；option.timestampMilliseconds会被timestampMilliseconds参数覆盖
    @param [out] savedFilePath 可选输出最终保存路径，传nullptr表示不需要
    @param [out] errorText 可选输出错误文本，传nullptr表示不需要
    @return 返回true表示截图保存成功
    */
    static bool extractFrameToFile(const std::string& filePath,
                                   int64_t timestampMilliseconds,
                                   const std::string& outputFilePath,
                                   const FFmpegCppFrameExtractOption& option = FFmpegCppFrameExtractOption(),
                                   std::string* savedFilePath = nullptr,
                                   std::string* errorText = nullptr);

    /** 转码视频文件
    @param [in] inputFilePath 输入视频完整路径
    @param [in] outputFilePath 输出视频完整路径，不能为空
    @param [in] option 转码选项，包括目标容器、视频编码、音频编码、分辨率、码率、CRF等
    @param [in] progressCallback 可选进度回调，返回false表示请求取消当前转码任务
    @param [in,out] userData 调用方自定义上下文指针，会原样传给progressCallback，库内部不读取也不释放
    @param [out] errorText 可选输出错误文本，传nullptr表示不需要
    @return 返回true表示转码成功
    */
    static bool transcodeVideo(const std::string& inputFilePath,
                               const std::string& outputFilePath,
                               const FFmpegCppTranscodeOption& option,
                               FFmpegCppProgressCallback progressCallback = nullptr,
                               void* userData = nullptr,
                               std::string* errorText = nullptr);

    /** 判断文件扩展名是否属于常见视频格式
    @param [in] filePath 文件路径或文件名
    @return 返回true表示扩展名属于mp4、mov、mkv、avi、flv、webm、m4v、3gp等常见视频格式
    */
    static bool isCommonVideoFileExtension(const std::string& filePath);

    /** 根据文件扩展名猜测视频mimeType
    @param [in] filePath 文件路径或文件名
    @return 返回mimeType；无法识别时返回application/octet-stream
    */
    static std::string guessVideoMimeTypeByFilePath(const std::string& filePath);

    /** 根据图片格式获取扩展名
    @param [in] imageFormat 图片格式
    @return 返回不带点号的扩展名，例如jpg、png、bmp；未知格式返回jpg
    */
    static std::string imageFormatToExtension(FFmpegCppImageFormat imageFormat);

    /** 根据图片格式获取mimeType
    @param [in] imageFormat 图片格式
    @return 返回image/jpeg、image/png或image/bmp；未知格式返回image/jpeg
    */
    static std::string imageFormatToMimeType(FFmpegCppImageFormat imageFormat);

    /** 根据视频编码枚举获取ffmpeg编码器建议名
    @param [in] videoCodec 视频编码枚举
    @return 返回编码器建议名，例如copy、libx264、libx265；未知时返回空字符串
    */
    static std::string videoCodecToEncoderName(FFmpegCppVideoCodec videoCodec);

    /** 根据音频编码枚举获取ffmpeg编码器建议名
    @param [in] audioCodec 音频编码枚举
    @return 返回编码器建议名，例如copy、aac、libmp3lame、libopus；未知时返回空字符串
    */
    static std::string audioCodecToEncoderName(FFmpegCppAudioCodec audioCodec);

    /** 把毫秒格式化成便于日志查看的时间文本
    @param [in] milliseconds 毫秒数，小于0时按0处理
    @return 返回HH:MM:SS.mmm格式文本
    */
    static std::string formatMilliseconds(int64_t milliseconds);
};