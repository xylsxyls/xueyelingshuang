#pragma once
#include "FFmpegCppMacro.h"
#include "FFmpegCppModels.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

/** Playback frame payload type
*/
enum FFmpegCppPlaybackFrameType
{
    // 当前帧不包含可用的音视频负载
    FFmpegCppPlaybackFrameTypeNone = 0,
    // 使用videoFrame中的BGRA图像和展示时间
    FFmpegCppPlaybackFrameTypeVideo = 1,
    // 使用audioFrame中的交错PCM和时间信息
    FFmpegCppPlaybackFrameTypeAudio = 2
};

/** Sequential playback read result
*/
enum FFmpegCppPlaybackReadResult
{
    // 已解码并返回一个音频帧或视频帧
    FFmpegCppPlaybackReadResultFrame = 0,
    // 输入及所有解码器尾帧均已读取完毕
    FFmpegCppPlaybackReadResultEnd = 1,
    // 读取或解码失败，详情由读取器lastError提供
    FFmpegCppPlaybackReadResultError = 2
};

/** 同步检查当前预览是否应提前终止，回调内不重入同一读取器
@param [in,out] userData 调用方上下文，库不读取也不释放，须在整个预览调用期间有效
@return true表示请求取消，false表示继续
*/
typedef bool (*FFmpegCppPlaybackCancelCallback)(void* userData);

/** Options used by single-frame preview decoding
*/
struct FFmpegCppAPI FFmpegCppPlaybackPreviewOption
{
public:
    // 预览最多解码的视频帧数，0使用内部默认预算
    size_t maxReadFrameCount;
    // 可选的同步取消回调，为空表示不检查外部取消
    FFmpegCppPlaybackCancelCallback cancelCallback;
    // 取消回调的借用上下文，预览调用期间必须有效
    void* cancelUserData;

public:
    /** Build default preview options
    */
    FFmpegCppPlaybackPreviewOption();

    /** Check whether the caller has canceled the preview request
    @return true if the request should stop
    */
    bool isCanceled() const;
};

/** Timing information collected while decoding a preview frame
*/
struct FFmpegCppAPI FFmpegCppPlaybackPreviewProfile
{
public:
    // 预览调用总耗时，单位微秒
    int64_t totalCostUs;
    // 定位耗时，单位微秒
    int64_t seekCostUs;
    // 解封装累计耗时，单位微秒
    int64_t demuxCostUs;
    // 解码累计耗时，单位微秒
    int64_t decodeCostUs;
    // 像素转换累计耗时，单位微秒
    int64_t convertCostUs;
    // 本次预览读取的输入包数
    size_t readPacketCount;
    // 本次预览解码的视频帧数
    size_t decodedVideoFrameCount;
    // 是否检测到调用方取消请求
    bool canceled;

public:
    /** Build an empty profile
    */
    FFmpegCppPlaybackPreviewProfile();

    /** Reset all counters to zero
    */
    void reset();
};

/** Playback reader open options
*/
struct FFmpegCppAPI FFmpegCppPlaybackOpenOption
{
public:
    // 是否查找并打开视频解码器
    bool decodeVideo;
    // 是否查找并打开音频解码器
    bool decodeAudio;
    // 目标PCM采样率，单位Hz，非正数时采用源采样率
    int32_t outputSampleRate;
    // 目标PCM声道数，非正数时按源声道数选择1或2声道
    int32_t outputChannels;
    // 预留的PCM位深选项，当前输出固定为16位
    int32_t outputBitsPerSample;
    // 无法从视频帧率推算时使用的正帧时长，单位100纳秒
    int64_t fallbackVideoFrameDuration100ns;

public:
    /** Build default playback options: BGRA video and S16 PCM audio
    */
    FFmpegCppPlaybackOpenOption();
};

/** BGRA video frame used by the playback reader
*/
struct FFmpegCppAPI FFmpegCppPlaybackVideoFrame
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 本帧持有的自上而下BGRA像素，行跨度由stride给出
    std::vector<unsigned char> bgraPixels;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 图像宽度，单位像素
    int32_t width;
    // 图像高度，单位像素
    int32_t height;
    // 每行像素的字节跨度，正常解码输出为width乘4
    int32_t stride;
    // 归一化展示起点，单位100纳秒
    int64_t timestamp100ns;
    // 本帧展示时长，单位100纳秒
    int64_t duration100ns;
    // 原始解码帧是否标记为关键帧
    bool keyFrame;

public:
    /** Build an empty video frame
    */
    FFmpegCppPlaybackVideoFrame();

    /** Clear all frame data
    */
    void reset();

    /** Check whether the frame has no usable pixel payload
    @return true if the frame is empty
    */
    bool empty() const;
};

/** Interleaved S16 PCM audio frame used by the playback reader
*/
struct FFmpegCppAPI FFmpegCppPlaybackAudioFrame
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 本帧持有的双字节有符号PCM，按声道交错排列
    std::vector<unsigned char> pcmData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // PCM采样率，单位Hz
    int32_t sampleRate;
    // PCM声道数
    int32_t channels;
    // PCM每样本位数，当前为16位
    int32_t bitsPerSample;
    // 归一化音频起点，单位100纳秒
    int64_t timestamp100ns;
    // 本帧PCM对应时长，单位100纳秒
    int64_t duration100ns;

public:
    /** Build an empty audio frame
    */
    FFmpegCppPlaybackAudioFrame();

    /** Clear all frame data
    */
    void reset();

    /** Check whether the frame has no usable audio payload
    @return true if the frame is empty
    */
    bool empty() const;
};

/** One decoded playback frame. The active payload is selected by type
*/
struct FFmpegCppAPI FFmpegCppPlaybackFrame
{
public:
    // 本次有效负载类型，None表示无可用帧
    FFmpegCppPlaybackFrameType type;
    // type为Video时有效的视频数据
    FFmpegCppPlaybackVideoFrame videoFrame;
    // type为Audio时有效的音频数据
    FFmpegCppPlaybackAudioFrame audioFrame;

public:
    /** Build an empty playback frame
    */
    FFmpegCppPlaybackFrame();

    /** Clear all frame data
    */
    void reset();
};

/** 基于FFmpeg的同步播放读取器，独占内部解码状态，调用方须串行访问
公开头不暴露FFmpeg原生类型；输出帧拥有自身数据，可在后续读取后继续持有
*/
class FFmpegCppAPI FFmpegCppPlaybackReader
{
public:
    /** Build a closed reader
    */
    FFmpegCppPlaybackReader();

    /** Destroy the reader and release all native resources
    */
    ~FFmpegCppPlaybackReader();

private:
    /** Copying is disabled because the native contexts own unique resources
    @param [in] other Another reader
    */
    FFmpegCppPlaybackReader(const FFmpegCppPlaybackReader& other);

    /** Assignment is disabled because the native contexts own unique resources
    @param [in] other Another reader
    @return Current reader
    */
    FFmpegCppPlaybackReader& operator=(const FFmpegCppPlaybackReader& other);

public:
    /** Open a media file
    @param [in] filePath UTF-8 encoded file path
    @param [in] option Open options
    @return true if the file was opened
    */
    bool open(const std::string& filePath, const FFmpegCppPlaybackOpenOption& option = FFmpegCppPlaybackOpenOption());

    /** Close the current media file. Calling this repeatedly is safe
    */
    void close();

    /** Check whether a media file is currently open
    @return true if a file is open
    */
    bool isOpen() const;

    /** Get the last operation error text
    @return Last error text
    */
    std::string lastError() const;

    /** Read media metadata
    @param [out] mediaInfo Media information output
    @return true if metadata was written
    */
    bool getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const;

    /** Get normalized media duration
    @return Duration in 100ns units
    */
    int64_t duration100ns() const;

    /** Get guessed video frame duration
    @return Frame duration in 100ns units
    */
    int64_t videoFrameDuration100ns() const;

    /** Get native video size
    @param [out] width Video width
    @param [out] height Video height
    @return true if the media has video
    */
    bool getVideoSize(int32_t* width, int32_t* height) const;

    /** Get output audio format
    @param [out] sampleRate PCM sample rate
    @param [out] channels PCM channel count
    @param [out] bitsPerSample PCM bit depth
    @return true if the media has audio
    */
    bool getAudioFormat(int32_t* sampleRate, int32_t* channels, int32_t* bitsPerSample) const;

    /** Check whether the media contains a video stream
    @return true if video is available
    */
    bool hasVideo() const;

    /** Check whether the media contains an audio stream
    @return true if audio is available
    */
    bool hasAudio() const;

    /** Seek to a normalized media position
    @param [in] position100ns Target position in 100ns units
    @return true if seeking succeeded
    */
    bool seek(int64_t position100ns);

    /** 精确定位纯视频reader并保留下一帧供read连续播放
    @param [in] position100ns 目标媒体位置，100纳秒
    @param [in] option 解码预算及取消条件
    @param [out] frame 包含目标的实际展示帧，以下一PTS修正结束边界
    @param [out] profile 可空，记录定位耗时及帧数
    @return 成功返回true；必须以decodeAudio=false打开
    */
    bool seekVideoFrame(int64_t position100ns, const FFmpegCppPlaybackPreviewOption& option,
        FFmpegCppPlaybackVideoFrame* frame, FFmpegCppPlaybackPreviewProfile* profile);

    /** 精确解析所在帧及下一PTS边界，不转换或返回像素；与同一reader的其他操作串行
    @param [in] position100ns 媒体时间，单位100纳秒
    @param [in] option 取消和解码预算
    @param [out] frame 真实边界及尺寸，像素为空，不能为空
    @param [out] profile 可空，实际解析耗时
    @return true表示真实帧边界已解析，失败返回false
    */
    bool readVideoFrameInfoAt(int64_t position100ns, const FFmpegCppPlaybackPreviewOption& option,
        FFmpegCppPlaybackVideoFrame* frame, FFmpegCppPlaybackPreviewProfile* profile);

    /** Read the next decoded video or audio frame
    @param [out] frame Decoded frame output
    @return Read result
    */
    FFmpegCppPlaybackReadResult read(FFmpegCppPlaybackFrame* frame);

    /** Seek and decode a nearby video preview frame
    @param [in] position100ns Target position in 100ns units
    @param [in] maxReadFrameCount Maximum decoded video frames after seek. Zero uses the internal default
    @param [out] frame Decoded video frame output
    @return true if a video frame was decoded
    */
    bool readVideoFrameAt(int64_t position100ns, size_t maxReadFrameCount, FFmpegCppPlaybackVideoFrame* frame);

    /** Seek and decode a nearby video preview frame with cancellation and timing details
    @param [in] position100ns Target position in 100ns units
    @param [in] option Preview behavior and cancellation callback
    @param [out] frame Decoded video frame output
    @param [out] profile Optional timing profile output
    @return true if a video frame was decoded
    */
    bool readVideoFrameAtEx(int64_t position100ns,
        const FFmpegCppPlaybackPreviewOption& option,
        FFmpegCppPlaybackVideoFrame* frame,
        FFmpegCppPlaybackPreviewProfile* profile);

    /** 可取消地读取相邻展示帧，复用有界原始帧缓存，仅转换目标帧
    @param [in] origin100ns 当前帧起点，单位100纳秒
    @param [in] direction 负数向左，正数向右，不允许0
    @param [in] option 取消回调配置
    @param [out] frame 相邻帧及完整展示时长
    @param [out] profile 统计及取消状态，可为空
    @return 成功返回true，失败或取消返回false
    */
    bool readAdjacentVideoFrameEx(int64_t origin100ns, int32_t direction,
        const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackVideoFrame* frame,
        FFmpegCppPlaybackPreviewProfile* profile);

private:
    // 本实例独占的FFmpegCppPlaybackReaderImpl，析构时释放，不暴露FFmpeg原生类型
    void* m_impl;
};