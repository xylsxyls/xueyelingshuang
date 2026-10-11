#pragma once

#include "KaraokeTimeline.h"
#include "KaraokeStorage.h"

#include <memory>
#include <stdint.h>
#include <vector>

/** 已录区间的内部数据，按时间线对象生命周期保留录音存储
*/
struct KaraokePcmCapturedRange
{
    // 原媒体区间起点，100纳秒单位
    int64_t m_mediaStart100ns;
    // 原媒体区间终点，100纳秒单位
    int64_t m_mediaEnd100ns;
    // 区间播放速率千分值，1000为1倍
    int32_t m_ratePermille;
    // 历史兼容的内存PCM共享所有权
    std::shared_ptr<std::vector<int16_t>> m_pcm;
    // 捕获PCM临时磁盘存储的共享所有权
    std::shared_ptr<KaraokeStorage> m_store;
    // 当前区间在临时存储中的字节偏移
    uint64_t m_storeOffset;
    // 当前区间在共享PCM中的起始帧偏移
    size_t m_sampleOffsetFrames;
    // 区间实际PCM帧数
    size_t m_frameCount;
};

/** 拥有录制区间及共享临时PCM存储，不创建UI或采集设备
*/
class KaraokePcmTimeline
{
public:
    /** 初始化KaraokePcmTimeline对象的状态，不转移输入借用对象的所有权
    */
    KaraokePcmTimeline();

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

    /** Fix the complete song mapping without changing captured samples
    @param [in] duration100ns Complete source duration
    @param [in] ratePermille Rate locked at recording start
    @return False for invalid duration/rate or incompatible captured ranges
    */
    bool configureSong(int64_t duration100ns, int32_t ratePermille);

    /** Return the configured or first captured rate; an empty legacy stream is 1x
    @return 录制速率千分值，1000表示1倍
    */
    int32_t recordingRate() const;

    /** Count retained captured frames, excluding all generated silence
    @return 保留的实际PCM帧数，不包含静音补全
    */
    uint64_t capturedFrameCount() const;

    /** 将时间线推进到目标媒体位置，未录制部分保持静音
    @param [in] mediaPosition100ns 原媒体时间，100纳秒单位
    @return 非负位置成功纳入时间线时返回true，失败保持原状态
    */
    bool advanceTo(int64_t mediaPosition100ns);

    /** 用新PCM或录制倍速覆盖指定媒体区间，保留两侧旧区间
    @param [in] mediaStart100ns 原媒体区间起点，100纳秒单位
    @param [in] mediaEnd100ns 原媒体区间终点，100纳秒单位，须大于起点
    @param [in] ratePermille 播放速率千分值，1000为1倍，范围由配置校验
    @param [in] interleavedPcm 借用的交错S16输入，长度为frameCount乘声道数
    @param [in] frameCount PCM帧数
    @return 合法区间覆盖成功返回true，非法范围或存储失败返回false
    */
    bool replaceCapturedRange(int64_t mediaStart100ns, int64_t mediaEnd100ns,
        int32_t ratePermille, const int16_t* interleavedPcm, size_t frameCount);

    /** 计算完整输出时间线的PCM帧数
    @param [out] frameCount 不可为空，接收PCM帧数
    @return 输出指针有效且计算未溢出时返回true
    */
    bool outputFrameCount(uint64_t* frameCount) const;

    /** 取得完整输出时间映射的值副本
    @return 时间映射区间的值副本
    */
    std::vector<KaraokeTimelineSegment> outputSegments() const;

    /** 读取时间线的输出PCM，未录制区间填静音
    @param [in] startFrame 输出PCM起始帧下标
    @param [in] frameCount PCM帧数
    @param [out] interleavedOutput 接收交错S16输出，至少frameCount乘声道数个元素
    @return 区间有效且所有PCM读取完成才返回true
    */
    bool renderOutputFrames(uint64_t startFrame, size_t frameCount,
        int16_t* interleavedOutput) const;

    /** 统计当前保留区间实际使用的PCM存储字节
    @return 保留区间的PCM字节数
    */
    size_t storedPcmBytes() const;

    /** 取得录音格式的采样率
    @return 每秒采样帧数，Hz
    */
    int32_t sampleRate() const;

    /** 取得录音格式的声道数
    @return 声道数
    */
    int32_t channels() const;

    /** 借用当前时间线映射，不转移所有权
    @return 对象内部时间线的借用引用，修改或销毁对象后不可使用
    */
    const KaraokeTimeline& timeline() const;

private:
    /** 将媒体位置映射到捕获区间内的PCM帧偏移
    @param [in] range 有效捕获区间的借用引用
    @param [in] mediaPosition100ns 原媒体时间，100纳秒单位
    @return 区间内部PCM帧偏移
    */
    static size_t frameOffsetForMediaPosition(const KaraokePcmCapturedRange& range,
        int64_t mediaPosition100ns);

    /** 将静音时长转换为PCM帧数
    @param [in] duration100ns 时长，100纳秒单位
    @param [in] sampleRate 采样率，Hz
    @return 按采样率换算的静音帧数
    */
    static uint64_t silenceFrameCount(int64_t duration100ns, int32_t sampleRate);

    /** 计算区间输出帧数并检查溢出
    @param [out] frameCount 不可为空，接收PCM帧数
    @return 输出指针有效且区间帧数未溢出时返回true
    */
    bool rangeFrameCount(uint64_t* frameCount) const;

    /** 统计当前保留区间实际使用的PCM存储字节
    @param [in] ranges 捕获区间集合的借用引用
    @param [out] byteCount 不可为空，接收PCM字节数
    @return 统计成功且未溢出返回true，失败不发布字节数
    */
    bool storedPcmBytes(const std::vector<KaraokePcmCapturedRange>& ranges,
        size_t* byteCount) const;

    /** Map an absolute source position to an absolute output sample boundary
    @param [in] mediaPosition100ns 原媒体时间，100纳秒单位
    @return 锁定速率下的输出PCM帧边界
    */
    uint64_t songFrame(int64_t mediaPosition100ns) const;

    /** Read microphone spans without stretching already captured PCM a second time
    @param [in] startFrame 输出PCM起始帧下标
    @param [in] frameCount PCM帧数
    @param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
    @return 成功读取歌曲PCM或生成静音时返回true
    */
    bool renderSongFrames(uint64_t startFrame, size_t frameCount, int16_t* output) const;

private:
    // 录音格式采样率，Hz
    int32_t m_sampleRate;
    // Complete source duration, independent of the last recorded position
    int64_t m_songDuration100ns;
    // Zero selects the legacy variable-rate timeline; otherwise the recording rate
    int32_t m_songRate;
    // 录音格式声道数
    int32_t m_channels;
    // PCM存储边界，字节，零按实现约定表示磁盘流
    size_t m_maxPcmBytes;
    // 拥有的原媒体与输出时间映射
    KaraokeTimeline m_timeline;
    // 保留的捕获区间，覆盖录制仅替换相交部分
    std::vector<KaraokePcmCapturedRange> m_capturedRanges;
    // 捕获PCM临时磁盘存储的共享所有权
    std::shared_ptr<KaraokeStorage> m_store;
};