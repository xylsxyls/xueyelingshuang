#pragma once
#include "FFmpegCppMacro.h"
#include <stdint.h>
#include <string>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct SwrContext;
struct SwsContext;
struct AVFrame;
class FFmpegCppNativeMediaWriter;

/** 顺序编码48kHz双声道S16音频和自上而下的BGRA视频
调用方串行访问；本实例拥有输出容器、编解码上下文和可选的Windows编码后备对象
*/
class FFmpegCppAPI FFmpegCppMediaWriter
{
public:
    /** 构造尚未打开输出文件的编码器
    */
    FFmpegCppMediaWriter();

    /** 释放全部输出资源；需要完整文件时应先显式调用finish
    */
    ~FFmpegCppMediaWriter();

    /** 创建MP3音频文件或包含H.264视频和AAC音频的MP4文件
    @param [in] path 输出文件路径，不可为空
    @param [in] video true创建MP4，false创建MP3
    @param [in] width 视频宽度，单位像素；视频模式要求2至16384的偶数，音频模式忽略
    @param [in] height 视频高度，单位像素；范围同width
    @return true表示输出已就绪；失败原因通过error读取
    */
    bool open(const std::string& path, bool video, int width, int height);

    /** 顺序追加48kHz双声道交错S16音频，时间戳由累计帧数生成
    @param [in] samples 调用期间借用的PCM，至少包含frames乘2个样本，不可为空
    @param [in] frames 每声道帧数，范围1至48000
    @return true表示音频已接收；不足编码块的数据暂存在本实例中
    */
    bool audio(const int16_t* samples, size_t frames);

    /** 追加一帧自上而下、无行填充的BGRA视频
    @param [in] pixels 调用期间借用的像素，不可为空
    @param [in] bytes 数据字节数，必须等于打开时的宽乘高乘4
    @param [in] time100ns 非负的输出时间戳，单位100纳秒
    @return true表示帧已交给编码器并写出当前可用包
    */
    bool video(const unsigned char* pixels, size_t bytes, int64_t time100ns);

    /** 排空音频尾块及编码器，写入容器尾并关闭输出文件
    @return true表示文件已完成；未打开或写入失败返回false
    */
    bool finish();

    /** 读取最近一次编码或写入失败的说明
    @return 错误文本副本
    */
    std::string error() const;

private:
    /** 禁止复制独占的输出上下文
    @param [in] other 另一编码器
    */
    FFmpegCppMediaWriter(const FFmpegCppMediaWriter& other);

    /** 禁止赋值独占的输出上下文
    @param [in] other 另一编码器
    @return 禁用操作，不提供实现
    */
    FFmpegCppMediaWriter& operator=(const FFmpegCppMediaWriter& other);

    /** 向编码器发送一帧并把当前可用包写入输出流
    @param [in] codec 已打开的编码上下文，调用期间借用
    @param [in] stream 输出流，调用期间借用
    @param [in] frame 待编码帧，不转移所有权；nullptr表示排空编码器
    @return true表示本次发送和写包成功
    */
    bool encode(AVCodecContext* codec, AVStream* stream, AVFrame* frame);

    /** 编码待处理PCM的前一块并在成功后推进音频时间轴
    @param [in] frames 每声道帧数，不超过m_pending中已有的完整双声道帧数
    @return true表示音频块已编码并从待处理缓存移除
    */
    bool audioBlock(size_t frames);

    /** 保存FFmpeg错误码对应的阶段说明
    @param [in] code FFmpeg错误码
    @param [in] stage 以零结尾的阶段文本，不可为空，仅在调用中借用
    @return 始终返回false，供失败分支直接返回
    */
    bool fail(int code, const char* stage);

    /** 释放上下文并关闭文件，不代替finish写入容器尾
    */
    void close();

private:
    // 本实例拥有的输出容器，关闭时释放
    AVFormatContext* m_format;
    // 本实例拥有的音频编码器
    AVCodecContext* m_audio;
    // 本实例拥有的视频编码器，纯音频模式为空
    AVCodecContext* m_video;
    // 输出容器拥有的音频流，本实例不单独释放
    AVStream* m_audioStream;
    // 输出容器拥有的视频流，纯音频模式为空
    AVStream* m_videoStream;
    // S16输入到编码器采样格式的重采样上下文
    SwrContext* m_resampler;
    // BGRA输入到视频编码格式的缩放和颜色转换上下文
    SwsContext* m_scaler;
    // 已提交编码的每声道音频帧数，用于生成时间戳
    int64_t m_audioFrames;
    // 输出是否已开始写入，finish后为false
    bool m_started;
    // 本实例拥有的Windows系统编码后备对象，其他平台保持为空
    FFmpegCppNativeMediaWriter* m_native;
    // 输出视频宽度，单位像素，纯音频模式为0
    int m_width;
    // 输出视频高度，单位像素，纯音频模式为0
    int m_height;
    // 尚未组成完整编码块的双声道交错S16样本
    std::vector<int16_t> m_pending;
    // 最近一次失败的阶段和错误文本
    std::string m_error;
};