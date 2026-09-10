#pragma once
#include "LumaPlayerCoreTypes.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <vector>

class FFmpegCppPlaybackReader;

/** 后台预解码下一轮的起始帧和PCM，完成后只读，可完整缓存短循环
*/
class PlayerLoopBuffer : public CTask
{
public:
    // 独立循环预备任务类别
    static const int32_t kTaskId = 2002;

    /** 保存本轮预备参数，实际打开与解码在DoTask执行
    @param [in] media 当前媒体信息的副本
    @param [in] start100ns 循环左边界，单位100纳秒
    @param [in] end100ns 循环排他右边界，单位100纳秒
    @param [in] maxBytes 缓存字节预算，至少允许保存一帧
    */
    PlayerLoopBuffer(const LumaPlayerMediaInfo& media, int64_t start100ns, int64_t end100ns, int64_t maxBytes);

    /** 释放尚未移交给播放引擎的读取器
    */
    virtual ~PlayerLoopBuffer();

    /** 在独立任务线程准备视频及音频，发布完成后不再修改缓存
    */
    virtual void DoTask();

    /** 请求协作取消，在每次解码返回后检查
    */
    virtual void StopTask();

    /** 判断已准备数据是否属于当前媒体和循环边界
    @param [in] media 当前媒体信息
    @param [in] start100ns 当前左边界
    @param [in] end100ns 当前排他右边界
    @return true表示完全匹配
    */
    bool matches(const LumaPlayerMediaInfo& media, int64_t start100ns, int64_t end100ns) const;

    /** 判断后台操作是否结束，返回true后才能读取缓存或转移读取器
    @return true表示完成，具体成败由result判断
    */
    bool isReady() const;

    /** 获取后台准备结果，只在isReady后调用
    @return 具体结果枚举
    */
    LumaPlayerCoreResult result() const;

    /** 判断视频和音频是否完整覆盖循环，完整缓存可重复使用
    @return true表示无需再次seek或解码即可循环
    */
    bool isComplete() const;

    /** 复制缓存中的视频帧，索引结束时返回End，未缓存后续数据返回NotOpen
    @param [in] index 缓存帧下标
    @param [out] frame 输出帧，不能为空
    @return 操作结果
    */
    LumaPlayerCoreResult videoFrame(size_t index, LumaPlayerVideoFrame* frame) const;

    /** 复制缓存中的音频帧，索引结束时返回End，未缓存后续数据返回NotOpen
    @param [in] index 缓存帧下标
    @param [out] frame 输出帧，不能为空
    @return 操作结果
    */
    LumaPlayerCoreResult audioFrame(size_t index, LumaPlayerAudioFrame* frame) const;

    /** 将已预热的视频读取器所有权移交播放线程，只调用一次
    @return 接收方负责释放的读取器，媒体无视频时为空
    */
    FFmpegCppPlaybackReader* takeVideoReader();

    /** 将已预热的音频读取器所有权移交播放线程，只调用一次
    @return 接收方负责释放的读取器，媒体无音频时为空
    */
    FFmpegCppPlaybackReader* takeAudioReader();

private:
    /** 预解码视频前缀，保留真实时间戳并丢弃A之前的预滚动帧
    @return 准备结果
    */
    LumaPlayerCoreResult prepareVideo();

    /** 预解码音频前缀，跨越边界的PCM交由播放引擎按采样裁剪
    @return 准备结果
    */
    LumaPlayerCoreResult prepareAudio();

private:
    // 不再修改的媒体参数副本
    LumaPlayerMediaInfo m_media;
    // 循环起点，单位100纳秒
    int64_t m_start100ns;
    // 循环排他结束边界，单位100纳秒
    int64_t m_end100ns;
    // 像素与PCM合计缓存预算
    int64_t m_maxBytes;
    // 当前缓存字节数
    int64_t m_cachedBytes;
    // 协作取消标记
    std::atomic<bool> m_exit;
    // 发布缓存完成状态，保证消费者看到全部数据
    std::atomic<bool> m_ready;
    // 具体准备结果
    LumaPlayerCoreResult m_result;
    // 缓存是否覆盖全部视频
    bool m_videoComplete;
    // 缓存是否覆盖全部音频
    bool m_audioComplete;
    // 独占的视频读取器，移交后置空
    FFmpegCppPlaybackReader* m_videoReader;
    // 独占的音频读取器，移交后置空
    FFmpegCppPlaybackReader* m_audioReader;
    // 保留A开始的实际展示帧
    std::vector<LumaPlayerVideoFrame> m_videoFrames;
    // 保留A开始的实际PCM片段
    std::vector<LumaPlayerAudioFrame> m_audioFrames;
};