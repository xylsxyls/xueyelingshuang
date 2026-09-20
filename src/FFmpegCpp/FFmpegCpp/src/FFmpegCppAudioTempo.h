#pragma once
#include "FFmpegCppMacro.h"
#include <stddef.h>
#include <stdint.h>

/** 实例级流式音频时间伸缩；调用方串行访问，内部持有并释放Sonic流 */
class FFmpegCppAPI FFmpegCppAudioTempo
{
public:
    /** 构造未初始化的流 */
    FFmpegCppAudioTempo();

    /** 释放流及未读样本 */
    ~FFmpegCppAudioTempo();

    /** 初始化16位交错PCM流，失败保持关闭
    @param [in] sampleRate 采样率，1000至500000
    @param [in] channels 声道数，1至32
    @return 初始化是否成功
    */
    bool init(int32_t sampleRate, int32_t channels);

    /** 丢弃未读样本并释放流，可重复调用 */
    void uninit();

    /** 设置速度并保留跨块历史，音调和采样率保持不变
    @param [in] ratePermille 50至20000，1000表示原速
    @return 参数及流是否有效
    */
    bool setRate(int32_t ratePermille);

    /** 追加完整交错PCM帧，数据只在调用中借用
    @param [in] samples 含frames乘声道数个样本
    @param [in] frames 每声道帧数，不超过INT_MAX
    @return 输入是否被接受
    */
    bool write(const int16_t* samples, size_t frames);

    /** 仅在流末尾排空算法延迟，不用于普通分块
    @return 排空是否成功
    */
    bool finish();

    /** 获取可读取的每声道帧数
    @return 帧数，未初始化为0
    */
    size_t available() const;

    /** 读取输出到调用方分配的缓冲
    @param [out] samples 至少容纳frames乘声道数个样本
    @param [in] frames 最大每声道帧数
    @return 实际读取帧数
    */
    size_t read(int16_t* samples, size_t frames);

private:
    /** 禁止复制有状态音频流 */
    FFmpegCppAudioTempo(const FFmpegCppAudioTempo& other);

    /** 禁止赋值有状态音频流 */
    FFmpegCppAudioTempo& operator=(const FFmpegCppAudioTempo& other);

private:
    // 第三方流只在实现文件解引用
    void* m_stream;
    // 限制第三方以int执行的帧数乘声道数运算
    int32_t m_channels;
};