#pragma once
#include "FFmpegCppPlaybackReader.h"
#include "FFmpegCppAdjacentReader.h"
#include <deque>
extern "C"
{
#include <ffmpeg/libavcodec/avcodec.h>
#include <ffmpeg/libavformat/avformat.h>
#include <ffmpeg/libswresample/swresample.h>
#include <ffmpeg/libswscale/swscale.h>
}

/** 播放读取器的私有状态，拥有解封装、解码、重采样及帧缓存资源
由FFmpegCppPlaybackReader独占；同一实例的全部操作由调用方串行执行
*/
struct FFmpegCppPlaybackReaderImpl
{
public:
    /** 初始化关闭状态，不打开媒体文件
    */
    FFmpegCppPlaybackReaderImpl();

    /** 关闭媒体并释放本实例持有的全部FFmpeg资源
    */
    ~FFmpegCppPlaybackReaderImpl();

    /** 关闭旧媒体并按选项打开新的解码会话
    @param [in] filePath UTF-8媒体路径，不可为空，调用中复制
    @param [in] option 视频解码及PCM输出选项，调用中复制
    @return true表示至少一个所选音视频流可播放；失败保留m_lastError
    */
    bool open(const std::string& filePath, const FFmpegCppPlaybackOpenOption& option);

    /** 丢弃排队帧并释放媒体资源，可重复调用
    */
    void close();

    /** 查询媒体上下文是否可用
    @return true表示打开标记和媒体上下文均有效
    */
    bool isOpen() const;

    /** 复制已打开媒体的元信息
    @param [out] mediaInfo 输出信息，不可为空
    @return true表示读取器已打开且信息已复制
    */
    bool getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const;

    /** 获取源视频尺寸，不执行缩放
    @param [out] width 宽度，单位像素，可为空
    @param [out] height 高度，单位像素，可为空
    @return true表示视频宽高均有效
    */
    bool getVideoSize(int32_t* width, int32_t* height) const;

    /** 获取解码后PCM的实际输出格式
    @param [out] sampleRate 每秒采样数，可为空
    @param [out] channels 声道数，可为空
    @param [out] bitsPerSample 每样本位数，可为空
    @return true表示音频解码器和输出格式有效
    */
    bool getAudioFormat(int32_t* sampleRate, int32_t* channels, int32_t* bitsPerSample) const;

    /** 定位到目标之前的关键帧并清除旧帧和重采样延迟
    @param [in] position100ns 归一化目标位置，单位100纳秒，按媒体范围裁剪
    @return true表示定位及解码状态重置成功，不表示已解码到精确展示帧
    */
    bool seek(int64_t position100ns);

    /** 读取排队帧或继续解封装和解码，直到得到一帧或终态
    @param [out] frame 由调用方持有的帧，不可为空，调用开始时重置
    @return 返回已得到帧、全部流结束或读取错误
    */
    FFmpegCppPlaybackReadResult read(FFmpegCppPlaybackFrame* frame);

    /** 定位并同步读取目标附近的视频预览帧
    @param [in] position100ns 目标位置，单位100纳秒
    @param [in] maxReadFrameCount 解码视频帧数预算，0使用内部默认值
    @param [out] frame 目标帧，不可为空
    @return true表示已得到预览帧，失败详情保存在m_lastError
    */
    bool readVideoFrameAt(int64_t position100ns, size_t maxReadFrameCount, FFmpegCppPlaybackVideoFrame* frame);

    /** 使用有界原始帧缓存读取当前帧前后相邻的真实展示帧
    @param [in] origin100ns 当前帧起点，单位100纳秒
    @param [in] direction 负数向前一帧，正数向后一帧，不允许0
    @param [in] option 取消条件及读取选项，调用期间借用
    @param [out] frame 相邻帧像素和展示区间，不可为空
    @param [out] profile 本次统计及取消状态，可为空
    @return true表示相邻帧已解析并完成像素转换
    */
    bool readAdjacentVideoFrameEx(int64_t origin100ns, int32_t direction,
        const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackVideoFrame* frame,
        FFmpegCppPlaybackPreviewProfile* profile);

    /** 按预算及取消条件同步解析目标帧和下一展示边界
    @param [in] position100ns 目标位置，单位100纳秒
    @param [in] option 解码预算及取消条件，调用期间借用
    @param [out] frame 目标帧，不可为空；metadataOnly时不含像素
    @param [out] profile 各阶段耗时及计数，可为空
    @param [in] keepFollowingFrame 是否保留后继帧供顺序播放，仅支持纯视频读取器
    @param [in] metadataOnly 是否仅解析帧身份而跳过像素转换
    @return true表示已得到有效目标帧，失败或取消返回false
    */
    bool readVideoFrameAtEx(int64_t position100ns,
        const FFmpegCppPlaybackPreviewOption& option,
        FFmpegCppPlaybackVideoFrame* frame,
        FFmpegCppPlaybackPreviewProfile* profile, bool keepFollowingFrame = false, bool metadataOnly = false);

private:
    /** 调用预览取消回调并更新取消状态和错误说明
    @param [in] option 当前预览选项
    @param [in,out] profile 已累计的统计，取消时置位canceled，可为空
    @return true表示调用方已请求取消
    */
    bool checkPreviewCanceled(const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackPreviewProfile* profile);

    /** 提取视频帧身份并在缺少时间戳时使用顺序推算值
    @param [in] frame 原始视频帧，不可为空，仅在调用中借用
    @param [in,out] nextVideoTimestamp100ns 缺失时间戳的回退值，成功后推进，不可为空
    @param [out] frameInfo 尺寸和展示时间，不包含像素，不可为空
    @return true表示帧元信息有效
    */
    bool fillPreviewVideoFrameInfo(const AVFrame* frame, int64_t* nextVideoTimestamp100ns, FFmpegCppPlaybackVideoFrame* frameInfo);

    /** 读取解码器当前可用视频帧并选取覆盖目标位置的候选帧
    @param [in] position100ns 目标位置，单位100纳秒
    @param [in] tolerance100ns 接受帧起点的额外容差，单位100纳秒
    @param [in] maxReadFrameCount 本次预览解码视频帧数上限
    @param [in,out] readVideoCount 已读取视频帧数，不可为空
    @param [in,out] nextVideoTimestamp100ns 缺失时间戳时的顺序回退值，不可为空
    @param [in,out] haveFrame 是否已有候选帧，不可为空
    @param [in,out] shouldStop 是否已越过目标边界，不可为空
    @param [in,out] bestRawFrame 接收候选原始帧引用的已分配AVFrame，不可为空
    @param [in,out] bestFrameInfo 候选帧元信息，不可为空
    @param [in] option 当前取消条件
    @param [in,out] profile 追加解码耗时和计数，可为空
    @param [in] keepFollowingFrame 是否缓存候选之后的帧以维持顺序播放
    @return true表示当前接收阶段成功，是否得到目标帧由haveFrame表示
    */
    bool receivePreviewFrames(int64_t position100ns,
        int64_t tolerance100ns,
        size_t maxReadFrameCount,
        size_t* readVideoCount,
        int64_t* nextVideoTimestamp100ns,
        bool* haveFrame,
        bool* shouldStop,
        AVFrame* bestRawFrame,
        FFmpegCppPlaybackVideoFrame* bestFrameInfo,
        const FFmpegCppPlaybackPreviewOption& option,
        FFmpegCppPlaybackPreviewProfile* profile, bool keepFollowingFrame);

    /** 将原始视频帧转换为紧密排列的BGRA像素
    @param [in] frame 原始帧，不可为空，调用期间借用
    @param [in,out] playbackFrame 保留时间信息并写入尺寸和像素，不可为空
    @return true表示颜色转换成功
    */
    bool convertVideoFrameToBgra(const AVFrame* frame, FFmpegCppPlaybackVideoFrame* playbackFrame);

    /** 为选中流创建并打开本实例拥有的解码器
    @param [in] streamIndex 输入容器中有效的流下标
    @param [in] video true表示视频，false表示音频并同时初始化PCM输出
    @return true表示对应解码链已就绪
    */
    bool openDecoder(int streamIndex, bool video);

    /** 按打开选项初始化S16交错PCM输出的重采样上下文
    @return true表示输出格式有效且重采样器已初始化
    */
    bool openAudioResampler();

    /** 根据已打开的容器和流刷新媒体信息、尺寸和时长缓存
    */
    void fillMediaInfo();

    /** 清除排队帧和排空标志，重设顺序时间戳推算起点
    @param [in] position100ns 新的归一化读取起点，单位100纳秒
    */
    void resetReadState(int64_t position100ns);

    /** 取出最早解码完成的排队帧
    @param [out] frame 接收队首帧，不可为空，由调用方持有输出数据
    @return true表示取到帧，队列为空返回false
    */
    bool popQueuedFrame(FFmpegCppPlaybackFrame* frame);

    /** 在输入结束后继续取得一个尚未排空的解码器输出
    @return true表示本轮排空成功，具体结束标志保存在各流状态中
    */
    bool drainOneDecoder();

    /** 提交一个输入包并接收当前可用帧
    @param [in] codecContext 对应流的解码器，不可为空
    @param [in] packet 输入包，调用期间借用，不转移所有权；nullptr表示排空解码器
    @param [in] video true使用视频帧路径，false使用音频帧路径
    @return true表示提交和接收均成功
    */
    bool decodePacket(AVCodecContext* codecContext, AVPacket* packet, bool video);

    /** 接收解码器当前可用帧并转换后加入输出队列
    @param [in] codecContext 已打开的解码器，不可为空
    @param [in] video true接收视频，false接收音频
    @return true表示接收成功或暂时无帧，解码错误返回false
    */
    bool receiveFrames(AVCodecContext* codecContext, bool video);

    /** 转换视频像素和时间信息并加入输出队列
    @param [in] frame 解码得到的视频帧，不可为空，不转移所有权
    @return true表示帧已转换并加入队列
    */
    bool queueVideoFrame(const AVFrame* frame);

    /** 重采样音频为交错S16并加入输出队列
    @param [in] frame 解码得到的音频帧，不可为空，不转移所有权
    @return true表示转换成功，无输出样本时也返回true
    */
    bool queueAudioFrame(const AVFrame* frame);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 当前媒体的UTF-8路径，打开时复制
    std::string m_filePath;
    // 最近一次读取、定位或转换失败的说明
    std::string m_lastError;
    // 已打开容器和全部流的媒体信息副本
    FFmpegCppMediaInfo m_mediaInfo;
    // 本次打开时复制的解码及PCM输出选项
    FFmpegCppPlaybackOpenOption m_option;
    // 已解码并持有自身像素或样本数据的顺序输出帧
    std::deque<FFmpegCppPlaybackFrame> m_frameQueue;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 相邻展示帧读取状态，独占原始帧引用并借用本实例解码器
    FFmpegCppAdjacentReader m_adjacent;
    // 本实例拥有的输入容器，关闭时释放
    AVFormatContext* m_formatContext;
    // 本实例拥有的视频解码器，无视频时为空
    AVCodecContext* m_videoCodecContext;
    // 本实例拥有的音频解码器，无音频时为空
    AVCodecContext* m_audioCodecContext;
    // 本实例拥有的BGRA颜色转换上下文
    SwsContext* m_scaleContext;
    // 本实例拥有的PCM重采样上下文
    SwrContext* m_resampleContext;
    // 重复使用的解封装包，关闭时释放
    AVPacket* m_packet;
    // 重复使用的解码帧，关闭时释放
    AVFrame* m_frame;
    // 所选视频流下标，无有效流时为-1
    int m_videoStreamIndex;
    // 所选音频流下标，无有效流时为-1
    int m_audioStreamIndex;
    // 原视频流时间戳的时间基
    AVRational m_videoTimeBase;
    // 原音频流时间戳的时间基
    AVRational m_audioTimeBase;
    // 视频流归一化时扣除的起点，单位100纳秒
    int64_t m_videoStartTime100ns;
    // 音频流归一化时扣除的起点，单位100纳秒
    int64_t m_audioStartTime100ns;
    // 归一化媒体总时长，单位100纳秒，打开后未知时回退为估算帧时长
    int64_t m_duration100ns;
    // 由帧率估算的视频帧时长，单位100纳秒
    int64_t m_videoFrameDuration100ns;
    // 视频缺少有效时间戳时的下一起点，单位100纳秒
    int64_t m_nextVideoTimestamp100ns;
    // 音频缺少有效时间戳时的下一起点，单位100纳秒
    int64_t m_nextAudioTimestamp100ns;
    // 源视频宽度，单位像素，无视频时为0
    int32_t m_videoWidth;
    // 源视频高度，单位像素，无视频时为0
    int32_t m_videoHeight;
    // PCM输出采样率，单位Hz
    int32_t m_audioSampleRate;
    // PCM输出声道数
    int32_t m_audioChannels;
    // PCM输出每样本位数，当前支持16位
    int32_t m_audioBitsPerSample;
    // 输入容器是否已完成打开
    bool m_isOpen;
    // 是否已开始向解码器发送输入结束标记
    bool m_drainStarted;
    // 视频解码器是否已输出全部尾帧
    bool m_videoDrained;
    // 音频解码器是否已输出全部尾帧
    bool m_audioDrained;
    // 当前读取状态下已解封装的包数，定位或关闭时重置
    uint64_t m_readPacketCount;
    // 当前读取状态下已接收的视频帧数
    uint64_t m_videoFrameCount;
    // 当前读取状态下已接收的音频帧数
    uint64_t m_audioFrameCount;
};