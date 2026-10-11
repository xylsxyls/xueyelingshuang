#pragma once

#include <stddef.h>
#include <stdint.h>
#include <limits>
#include <vector>

/** Pure C++ helpers for microphone gain, mute, monitoring and accompaniment mix
*/
class KaraokePcmProcessor
{
public:
    /** Apply software gain to recording PCM and independently produce monitor PCM
    @param [in] input Interleaved signed 16-bit samples
    @param [in] sampleCount Number of interleaved samples
    @param [in] gainPercent Actual PCM microphone gain, 0 to 5000 percent
    @param [in] muted Microphone mute state; muted input is silent on both outputs
    @param [in] monitoring Whether microphone audio is audible on the player output
    @param [out] recording Receives gained samples for the recording timeline
    @param [out] monitor Receives gained samples for playback monitoring, or silence
    @return false for invalid parameters; output buffers remain unchanged
    */
    static bool processMicrophone(const int16_t* input, size_t sampleCount,
        int32_t gainPercent, bool muted, bool monitoring,
        int16_t* recording, int16_t* monitor);

    /** Saturating sum of two interleaved S16 sample arrays
    @param [in] accompaniment 借用的背景音采样数组，至少sampleCount个元素
    @param [in] microphone 借用的麦克风S16采样数组，至少sampleCount个元素
    @param [in] sampleCount 交错样本个数，不是字节数
    @param [out] output 接收饱和后的混音S16，至少sampleCount个元素
    @return 参数有效并完成饱和相加才返回true
    */
    static bool mixS16(const int16_t* accompaniment, const int16_t* microphone,
        size_t sampleCount, int16_t* output);

    /** Convert microphone monitor PCM to a different S16 rate/channel layout
    @param [in] input 借用的PCM输入，长度由采样数或帧数乘声道数决定
    @param [in] inputFrames 输入PCM帧数
    @param [in] inputRate 输入采样率，Hz，必须大于零
    @param [in] inputChannels 输入声道数，1或2
    @param [in] outputRate 输出采样率，Hz，必须大于零
    @param [in] outputChannels 输出声道数，1或2
    @param [in,out] frameRemainder 跨块采样帧换算余数，初始为零，必须小于inputRate
    @param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
    @return 转换成功返回true，失败不替换输出或帧余数
    */
    static bool convertMonitorS16(const int16_t* input, size_t inputFrames,
        int32_t inputRate, int32_t inputChannels, int32_t outputRate,
        int32_t outputChannels, uint64_t* frameRemainder,
        std::vector<int16_t>* output);

private:
    /** 将单个S16采样乘以软件增益并饱和限制
    @param [in] sample S16单采样幅度
    @param [in] gainPercent 麦克风软件增益百分比，0至5000，界面显示值与此不同
    @return 饱和到S16范围的采样值
    */
    static int16_t applyGain(int16_t sample, int32_t gainPercent);
};