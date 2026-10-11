#pragma once

#include <stdint.h>
#include <vector>

/** 完整歌曲时间映射区间的值类型，不拥有音频设备
*/
struct KaraokeTimelineSegment
{
    // 原媒体区间起点，100纳秒单位
    int64_t m_mediaStart100ns;
    // 原媒体区间终点，100纳秒单位
    int64_t m_mediaEnd100ns;
    // 导出区间起点，100纳秒单位
    int64_t m_outputStart100ns;
    // 导出区间终点，100纳秒单位
    int64_t m_outputEnd100ns;
    // 区间播放速率千分值，1000为1倍
    int32_t m_ratePermille;
    // 区间是否含已录制的麦克风PCM
    bool m_hasMicrophoneAudio;
};

/** 已录区间的内部数据，按时间线对象生命周期保留录音存储
*/
struct KaraokeTimelineCapturedRange
{
    // 捕获区间原媒体起点，100纳秒单位
    int64_t m_start100ns;
    // 捕获区间原媒体终点，100纳秒单位
    int64_t m_end100ns;
    // 区间播放速率千分值，1000为1倍
    int32_t m_ratePermille;
};

class KaraokeTimeline
{
public:
    /** 初始化KaraokeTimeline对象的状态，不转移输入借用对象的所有权
    */
    KaraokeTimeline();

    /** 交换时间线状态，保持区间与完整时长一致
    @param [in,out] other 要交换状态的另一时间线对象
    */
    void swap(KaraokeTimeline& other);

    /** 清除已录制区间，保留对象格式配置
    */
    void clear();

    /** 将时间线推进到目标媒体位置，未录制部分保持静音
    @param [in] mediaPosition100ns 原媒体时间，100纳秒单位
    @return 非负位置成功纳入时间线时返回true，失败保持原状态
    */
    bool advanceTo(int64_t mediaPosition100ns);

    /** 用新PCM或录制倍速覆盖指定媒体区间，保留两侧旧区间
    @param [in] mediaStart100ns 原媒体区间起点，100纳秒单位
    @param [in] mediaEnd100ns 原媒体区间终点，100纳秒单位，须大于起点
    @param [in] ratePermille 播放速率千分值，1000为1倍，范围由配置校验
    @return 合法区间覆盖成功返回true，非法范围或存储失败返回false
    */
    bool replaceCapturedRange(int64_t mediaStart100ns, int64_t mediaEnd100ns,
        int32_t ratePermille);

    /** 取得当前媒体时间线的完整时长
    @return 媒体时长，100纳秒单位
    */
    int64_t mediaDuration100ns() const;

    /** 计算倍速映射后的输出时长
    @param [out] duration100ns 不可为空，接收时长，100纳秒单位
    @return true表示换算成功，溢出或无效参数返回false
    */
    bool outputDuration100ns(int64_t* duration100ns) const;

    /** 将原媒体位置换算为输出时间
    @param [in] mediaTime100ns 原媒体时间，100纳秒单位
    @param [out] outputTime100ns 不可为空，接收输出时间，100纳秒单位
    @return 时间位于映射范围且换算成功时返回true
    */
    bool mediaToOutputTime100ns(int64_t mediaTime100ns, int64_t* outputTime100ns) const;

    /** 将输出时间换算为原媒体位置
    @param [in] outputTime100ns 输出时间，100纳秒单位
    @param [out] mediaTime100ns 不可为空，接收原媒体时间，100纳秒单位
    @return 输出时间位于映射范围且换算成功时返回true
    */
    bool outputToMediaTime100ns(int64_t outputTime100ns, int64_t* mediaTime100ns) const;

    /** 取得录制与静音区间的完整时间映射副本
    @return 完整映射的值副本
    */
    std::vector<KaraokeTimelineSegment> segments() const;

private:
    /** 有界缩放时间，溢出时不发布输出
    @param [in] value 待缩放的非负时间值
    @param [in] numerator 时间缩放分子，必须大于零
    @param [in] denominator 时间缩放分母，必须大于零
    @param [out] scaled 接收成功换算的时间值，不可为空
    @return 非负时间及正缩放比例未溢出时返回true
    */
    static bool scaleTime(int64_t value, int32_t numerator, int32_t denominator,
        int64_t* scaled);

private:
    // 当前媒体时间线时长，100纳秒单位
    int64_t m_mediaDuration100ns;
    // 保留的捕获区间，覆盖录制仅替换相交部分
    std::vector<KaraokeTimelineCapturedRange> m_capturedRanges;
};