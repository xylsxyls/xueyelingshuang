#pragma once
#include "FFmpegCppMacro.h"
#include "FFmpegCppModels.h"

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

/**
 * Playback frame payload type.
 */
enum FFmpegCppPlaybackFrameType
{
    FFmpegCppPlaybackFrameTypeNone = 0,
    FFmpegCppPlaybackFrameTypeVideo = 1,
    FFmpegCppPlaybackFrameTypeAudio = 2
};

/**
 * Sequential playback read result.
 */
enum FFmpegCppPlaybackReadResult
{
    FFmpegCppPlaybackReadResultFrame = 0,
    FFmpegCppPlaybackReadResultEnd = 1,
    FFmpegCppPlaybackReadResultError = 2
};
/**
 * Preview cancel callback. Return true when the current preview request should stop early.
 */
typedef bool (*FFmpegCppPlaybackCancelCallback)(void* userData);

/**
 * Options used by single-frame preview decoding.
 */
struct FFmpegCppAPI FFmpegCppPlaybackPreviewOption
{
public:
    size_t maxReadFrameCount;
    FFmpegCppPlaybackCancelCallback cancelCallback;
    void* cancelUserData;

public:
    /**
     * Build default preview options.
     */
    FFmpegCppPlaybackPreviewOption();

    /**
     * Check whether the caller has canceled the preview request.
     * @return true if the request should stop.
     */
    bool isCanceled() const;
};

/**
 * Timing information collected while decoding a preview frame.
 */
struct FFmpegCppAPI FFmpegCppPlaybackPreviewProfile
{
public:
    int64_t totalCostUs;
    int64_t seekCostUs;
    int64_t demuxCostUs;
    int64_t decodeCostUs;
    int64_t convertCostUs;
    size_t readPacketCount;
    size_t decodedVideoFrameCount;
    bool canceled;

public:
    /**
     * Build an empty profile.
     */
    FFmpegCppPlaybackPreviewProfile();

    /**
     * Reset all counters to zero.
     */
    void reset();
};

/**
 * Playback reader open options.
 */
struct FFmpegCppAPI FFmpegCppPlaybackOpenOption
{
public:
    bool decodeVideo;
    bool decodeAudio;
    int32_t outputSampleRate;
    int32_t outputChannels;
    int32_t outputBitsPerSample;
    int64_t fallbackVideoFrameDuration100ns;

public:
    /**
     * Build default playback options: BGRA video and S16 PCM audio.
     */
    FFmpegCppPlaybackOpenOption();
};

/**
 * BGRA video frame used by the playback reader.
 */
struct FFmpegCppAPI FFmpegCppPlaybackVideoFrame
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<unsigned char> bgraPixels;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    int32_t width;
    int32_t height;
    int32_t stride;
    int64_t timestamp100ns;
    int64_t duration100ns;
    bool keyFrame;

public:
    /**
     * Build an empty video frame.
     */
    FFmpegCppPlaybackVideoFrame();

    /**
     * Clear all frame data.
     */
    void reset();

    /**
     * Check whether the frame has no usable pixel payload.
     * @return true if the frame is empty.
     */
    bool empty() const;
};

/**
 * Interleaved S16 PCM audio frame used by the playback reader.
 */
struct FFmpegCppAPI FFmpegCppPlaybackAudioFrame
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<unsigned char> pcmData;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    int32_t sampleRate;
    int32_t channels;
    int32_t bitsPerSample;
    int64_t timestamp100ns;
    int64_t duration100ns;

public:
    /**
     * Build an empty audio frame.
     */
    FFmpegCppPlaybackAudioFrame();

    /**
     * Clear all frame data.
     */
    void reset();

    /**
     * Check whether the frame has no usable audio payload.
     * @return true if the frame is empty.
     */
    bool empty() const;
};

/**
 * One decoded playback frame. The active payload is selected by type.
 */
struct FFmpegCppAPI FFmpegCppPlaybackFrame
{
public:
    FFmpegCppPlaybackFrameType type;
    FFmpegCppPlaybackVideoFrame videoFrame;
    FFmpegCppPlaybackAudioFrame audioFrame;

public:
    /**
     * Build an empty playback frame.
     */
    FFmpegCppPlaybackFrame();

    /**
     * Clear all frame data.
     */
    void reset();
};

/**
 * FFmpeg-backed playback reader.
 *
 * The public header intentionally exposes no native FFmpeg headers or types.
 */
class FFmpegCppAPI FFmpegCppPlaybackReader
{
public:
    /**
     * Build a closed reader.
     */
    FFmpegCppPlaybackReader();

    /**
     * Destroy the reader and release all native resources.
     */
    ~FFmpegCppPlaybackReader();

private:
    /**
     * Copying is disabled because the native contexts own unique resources.
     * @param [in] other Another reader.
     */
    FFmpegCppPlaybackReader(const FFmpegCppPlaybackReader& other);

    /**
     * Assignment is disabled because the native contexts own unique resources.
     * @param [in] other Another reader.
     * @return Current reader.
     */
    FFmpegCppPlaybackReader& operator=(const FFmpegCppPlaybackReader& other);

public:
    /**
     * Open a media file.
     * @param [in] filePath UTF-8 encoded file path.
     * @param [in] option Open options.
     * @return true if the file was opened.
     */
    bool open(const std::string& filePath, const FFmpegCppPlaybackOpenOption& option = FFmpegCppPlaybackOpenOption());

    /**
     * Close the current media file. Calling this repeatedly is safe.
     */
    void close();

    /**
     * Check whether a media file is currently open.
     * @return true if a file is open.
     */
    bool isOpen() const;

    /**
     * Get the last operation error text.
     * @return Last error text.
     */
    std::string lastError() const;

    /**
     * Read media metadata.
     * @param [out] mediaInfo Media information output.
     * @return true if metadata was written.
     */
    bool getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const;

    /**
     * Get normalized media duration.
     * @return Duration in 100ns units.
     */
    int64_t duration100ns() const;

    /**
     * Get guessed video frame duration.
     * @return Frame duration in 100ns units.
     */
    int64_t videoFrameDuration100ns() const;

    /**
     * Get native video size.
     * @param [out] width Video width.
     * @param [out] height Video height.
     * @return true if the media has video.
     */
    bool getVideoSize(int32_t* width, int32_t* height) const;

    /**
     * Get output audio format.
     * @param [out] sampleRate PCM sample rate.
     * @param [out] channels PCM channel count.
     * @param [out] bitsPerSample PCM bit depth.
     * @return true if the media has audio.
     */
    bool getAudioFormat(int32_t* sampleRate, int32_t* channels, int32_t* bitsPerSample) const;

    /**
     * Check whether the media contains a video stream.
     * @return true if video is available.
     */
    bool hasVideo() const;

    /**
     * Check whether the media contains an audio stream.
     * @return true if audio is available.
     */
    bool hasAudio() const;

    /**
     * Seek to a normalized media position.
     * @param [in] position100ns Target position in 100ns units.
     * @return true if seeking succeeded.
     */
    bool seek(int64_t position100ns);

    /**
     * Read the next decoded video or audio frame.
     * @param [out] frame Decoded frame output.
     * @return Read result.
     */
    FFmpegCppPlaybackReadResult read(FFmpegCppPlaybackFrame* frame);

    /**
     * Seek and decode a nearby video preview frame.
     * @param [in] position100ns Target position in 100ns units.
     * @param [in] maxReadFrameCount Maximum decoded video frames after seek. Zero uses the internal default.
     * @param [out] frame Decoded video frame output.
     * @return true if a video frame was decoded.
     */
    bool readVideoFrameAt(int64_t position100ns, size_t maxReadFrameCount, FFmpegCppPlaybackVideoFrame* frame);

    /**
     * Seek and decode a nearby video preview frame with cancellation and timing details.
     * @param [in] position100ns Target position in 100ns units.
     * @param [in] option Preview behavior and cancellation callback.
     * @param [out] frame Decoded video frame output.
     * @param [out] profile Optional timing profile output.
     * @return true if a video frame was decoded.
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
    void* m_impl;
};