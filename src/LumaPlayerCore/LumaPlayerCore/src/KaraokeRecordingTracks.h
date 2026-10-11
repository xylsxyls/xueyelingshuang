#pragma once

#include "KaraokePcmTimeline.h"

/** Independent microphone lanes; monitoring never waits for this recording mix
*/
class KaraokeRecordingTracks
{
public:
    /** 初始化对象及其工作资源
    @param [in] sampleRate 采样率，Hz
    @param [in] channels 声道数
    @param [in] maxPcmBytes PCM存储上限，字节，零按入口约定使用磁盘存储
    @return true表示初始化成功
    */
    bool init(int32_t sampleRate, int32_t channels, size_t maxPcmBytes);

    /** 清除已录制区间，保留对象格式配置
    */
    void clear();

    /** 按锁定的录制倍速建立完整歌曲映射，保留已有录音区间
    @param [in] duration100ns 时长，100纳秒单位
    @param [in] ratePermille 播放速率千分值，1000为1倍，范围由配置校验
    @return 完整时长及锁定倍速可映射且与旧录音兼容时返回true
    */
    bool configureSong(int64_t duration100ns, int32_t ratePermille);

    /** 取得已锁定或首个录制区间的倍速，无录音时返回1倍
    @return 录制速率千分值，1000表示1倍
    */
    int32_t recordingRate() const;

    /** 统计保留的实际录音帧，不含补全的静音
    @return 保留的实际PCM帧数，不包含静音补全
    */
    uint64_t capturedFrameCount() const;

    /** 将时间线推进到目标媒体位置，未录制部分保持静音
    @param [in] mediaPosition100ns 原媒体时间，100纳秒单位
    @return 非负位置成功纳入时间线时返回true，失败保持原状态
    */
    bool advanceTo(int64_t mediaPosition100ns);

    /** 用新PCM或录制倍速覆盖指定媒体区间，保留两侧旧区间
    @param [in] start 原媒体区间起点，100纳秒单位
    @param [in] end 原媒体区间终点，100纳秒单位
    @param [in] rate 录制速率千分值，1000为1倍
    @param [in] pcm 借用的交错S16输入，至少frames乘声道数个元素，调用时复制
    @param [in] frames PCM帧数，不是交错样本数或字节数
    @param [in] microphone 麦克风槽位索引，0至2
    @return 合法区间覆盖成功返回true，非法范围或存储失败返回false
    */
    bool replaceCapturedRange(int64_t start, int64_t end, int32_t rate,
        const int16_t* pcm, size_t frames, int32_t microphone = 0);

    /** 计算完整输出时间线的PCM帧数
    @param [out] frames 不可为空，接收完整输出PCM帧数
    @return 输出指针有效且计算未溢出时返回true
    */
    bool outputFrameCount(uint64_t* frames) const;

    /** 取得完整输出时间映射的值副本
    @return 时间映射区间的值副本
    */
    std::vector<KaraokeTimelineSegment> outputSegments() const;

    /** 读取时间线的输出PCM，未录制区间填静音
    @param [in] start 输出PCM时间线的起始帧下标
    @param [in] frames PCM帧数，不是交错样本数或字节数
    @param [out] output 接收混合后的交错S16，至少frames乘声道数个元素
    @return 区间有效且所有PCM读取完成才返回true
    */
    bool renderOutputFrames(uint64_t start, size_t frames, int16_t* output) const;

    /** 取得录音格式的采样率
    @return 每秒采样帧数，Hz
    */
    int32_t sampleRate() const;

    /** 取得录音格式的声道数
    @return 声道数
    */
    int32_t channels() const;

private:
    // 三个麦克风独立录音轨，导出时才混合
    KaraokePcmTimeline m_tracks[3];
};