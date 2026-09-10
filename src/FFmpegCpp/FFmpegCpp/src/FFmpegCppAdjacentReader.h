#pragma once
#include "FFmpegCppPlaybackReader.h"
#include <deque>
#include <stdint.h>

struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;

/** 相邻展示帧读取状态，借用解码器，持有有界原始帧缓存，不持有平台或业务对象
*/
class FFmpegCppAdjacentReader
{
public:
    /** 初始化空缓存
    */
    FFmpegCppAdjacentReader();

    /** 释放本对象持有的帧引用，属于局部资源所有权清理
    */
    ~FFmpegCppAdjacentReader();

    /** 外部seek、换媒体或关闭前清除缓存和顺序读取状态
    */
    void reset();

    /** 查找真实相邻展示帧，仅返回借用的原始帧，颜色转换由调用方执行
    @param [in] format 已打开的媒体上下文，不为空
    @param [in] codec 视频解码器，不为空且仅由当前线程访问
    @param [in] streamIndex 视频流下标
    @param [in] start100ns 媒体归一化起点，单位100纳秒
    @param [in] duration100ns 媒体结束位置，单位100纳秒
    @param [in] origin100ns 当前帧展示起点，单位100纳秒
    @param [in] direction 负数向左，正数向右，不允许0
    @param [in] option 取消回调配置，调用期间有效
    @param [out] info 帧身份和真实展示区间，不含像素
    @param [out] profile 计数和取消状态，可为空
    @return 原始帧借用指针，下一次read/reset前有效，失败返回nullptr
    */
    const AVFrame* read(AVFormatContext* format, AVCodecContext* codec, int32_t streamIndex,
        int64_t start100ns, int64_t duration100ns, int64_t origin100ns, int32_t direction,
        const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackVideoFrame* info,
        FFmpegCppPlaybackPreviewProfile* profile);

private:
    /** 禁止复制拥有的原始帧引用
    @param [in] other 另一读取状态
    */
    FFmpegCppAdjacentReader(const FFmpegCppAdjacentReader& other);

    /** 禁止赋值拥有的原始帧引用
    @param [in] other 另一读取状态
    @return 当前对象
    */
    FFmpegCppAdjacentReader& operator=(const FFmpegCppAdjacentReader& other);

    /** 检查业务取消标记并写入统计
    @param [in] option 取消回调
    @param [out] profile 统计，可为空
    @return true表示取消
    */
    bool canceled(const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackPreviewProfile* profile);

    /** 获取帧在归一化时间轴的展示起点
    @param [in] frame 原始帧，不为空
    @param [in] format 媒体上下文
    @param [in] streamIndex 视频流下标
    @param [in] start100ns 归一化起点
    @return 100纳秒起点，缺失时间戳返回-1
    */
    int64_t timestamp(const AVFrame* frame, AVFormatContext* format, int32_t streamIndex, int64_t start100ns);

    /** 解码下一张原始视频帧，保留解码器参考状态，不进行颜色转换
    @param [in] format 媒体上下文
    @param [in] codec 视频解码器
    @param [in] streamIndex 视频流下标
    @param [in] option 取消回调
    @param [out] profile 统计，可为空
    @return 成功返回由调用方释放的帧，结束或失败返回nullptr
    */
    AVFrame* decodeNext(AVFormatContext* format, AVCodecContext* codec, int32_t streamIndex,
        const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackPreviewProfile* profile);

private:
    // 缓存帧数上限
    static const size_t kMaxCachedFrames = 16;
    // 原始像素缓存目标，不含解码器内部参考帧和行对齐开销
    static const int64_t kCacheBudgetBytes = 67108864;
    // 原始帧引用，最多保留16帧且以64MiB为目标，至少保留3帧
    std::deque<AVFrame*> m_frames;
    // 是否已给解码器发送文件结束标记
    bool m_draining;
    // 解码器是否已经输出全部帧
    bool m_ended;
};