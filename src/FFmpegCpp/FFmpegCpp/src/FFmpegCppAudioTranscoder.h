#pragma once
#include <stdint.h>
#include <string>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVPacket;
struct AVFrame;
struct AVAudioFifo;
struct SwrContext;

/** 单次转码拥有的AAC解码、重采样、编码状态，仅由转码执行者访问
*/
class FFmpegCppAudioTranscoder
{
public:
    /** 初始化为空的音频链
    */
    FFmpegCppAudioTranscoder();

    /** 释放本实例全部音频上下文
    */
    ~FFmpegCppAudioTranscoder();

    /** 在写容器头之前建立最佳音轨的AAC输出，无音轨时成功且不添加流
    @param [in] input 已打开的输入容器，不可为空，所选流须在本次转码期间有效
    @param [in] output 借用输出容器，不可为空，必须活到实例析构
    @param [in] bitRateKbps 存在音轨时要求16至512，单位Kbps
    @param [out] error 失败说明
    @return 初始化是否成功
    */
    bool open(AVFormatContext* input, AVFormatContext* output, int32_t bitRateKbps, std::string& error);

    /** 消费一个音频包，其他流保持不变
    @param [in] packet 调用期间借用的输入包，不可为空，不释放输入包
    @param [out] error 失败说明
    @return 解码及输出成功
    */
    bool process(AVPacket* packet, std::string& error);

    /** 排空解码器、重采样器和编码器
    @param [out] error 失败说明
    @return 所有尾部样本成功写入
    */
    bool finish(std::string& error);

private:
    /** 禁止复制音频资源
    @param [in] other 原实例
    */
    FFmpegCppAudioTranscoder(const FFmpegCppAudioTranscoder& other);

    /** 禁止赋值音频资源
    @param [in] other 原实例
    @return 不可调用
    */
    FFmpegCppAudioTranscoder& operator=(const FFmpegCppAudioTranscoder& other);

    /** 读取解码帧并向FIFO追加重采样结果
    @param [out] error 失败说明
    @return 是否成功
    */
    bool receive(std::string& error);

    /** 编码FIFO中完整或最终不足一帧的样本
    @param [in] final 是否允许输出尾部不足一帧的样本
    @param [out] error 失败说明
    @return 是否成功
    */
    bool drain(bool final, std::string& error);

    /** 发送编码帧并把全部可用包写入容器
    @param [in] frame 音频帧，为空表示排空
    @param [out] error 失败说明
    @return 是否成功
    */
    bool encode(AVFrame* frame, std::string& error);

    /** 转换并追加PCM，可用空帧排空重采样器
    @param [in] frame 解码帧或空
    @param [out] error 失败说明
    @return 是否成功
    */
    bool convert(AVFrame* frame, std::string& error);

private:
    // 借用的输出容器，须存活至本实例析构
    AVFormatContext* m_output;
    // 借用的最佳输入音频流，由输入容器持有
    AVStream* m_inputStream;
    // 借用的AAC输出流，由输出容器持有
    AVStream* m_outputStream;
    // 本实例拥有的源音频解码器
    AVCodecContext* m_decoder;
    // 本实例拥有的AAC编码器
    AVCodecContext* m_encoder;
    // 本实例拥有的采样格式和声道转换上下文
    SwrContext* m_resampler;
    // 本实例拥有的重采样结果FIFO，以编码器采样格式存储
    AVAudioFifo* m_fifo;
    // 仅转码所选最佳音轨
    int32_t m_streamIndex;
    // 下一编码帧的PTS，以编码器1/48000秒时间基计数，保留源音视频起点差
    int64_t m_nextPts;
    // 是否已从首个有效输入帧建立输出时间起点
    bool m_timed;
};