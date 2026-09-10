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

/** FFmpegCppPlaybackReader私有实现
*/
struct FFmpegCppPlaybackReaderImpl
{
public:
    /** 内部FFmpegCppPlaybackReaderImpl操作，调用方须串行访问此读取器 */
    FFmpegCppPlaybackReaderImpl();

    /** 内部~FFmpegCppPlaybackReaderImpl操作，调用方须串行访问此读取器 */
    ~FFmpegCppPlaybackReaderImpl();

    /** 内部open操作，调用方须串行访问此读取器 */
    bool open(const std::string& filePath, const FFmpegCppPlaybackOpenOption& option);

    /** 内部close操作，调用方须串行访问此读取器 */
    void close();

    /** 内部isOpen操作，调用方须串行访问此读取器 */
    bool isOpen() const;

    /** 内部getMediaInfo操作，调用方须串行访问此读取器 */
    bool getMediaInfo(FFmpegCppMediaInfo* mediaInfo) const;

    /** 内部getVideoSize操作，调用方须串行访问此读取器 */
    bool getVideoSize(int32_t* width, int32_t* height) const;

    /** 内部getAudioFormat操作，调用方须串行访问此读取器 */
    bool getAudioFormat(int32_t* sampleRate, int32_t* channels, int32_t* bitsPerSample) const;

    /** 内部seek操作，调用方须串行访问此读取器 */
    bool seek(int64_t position100ns);

    /** 内部read操作，调用方须串行访问此读取器 */
    FFmpegCppPlaybackReadResult read(FFmpegCppPlaybackFrame* frame);

    /** 内部readVideoFrameAt操作，调用方须串行访问此读取器 */
    bool readVideoFrameAt(int64_t position100ns, size_t maxReadFrameCount, FFmpegCppPlaybackVideoFrame* frame);

    /** 内部readAdjacentVideoFrameEx操作，调用方须串行访问此读取器 */
    bool readAdjacentVideoFrameEx(int64_t origin100ns, int32_t direction,
        const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackVideoFrame* frame,
        FFmpegCppPlaybackPreviewProfile* profile);

    /** 内部readVideoFrameAtEx操作，调用方须串行访问此读取器 */
    bool readVideoFrameAtEx(int64_t position100ns,
                            const FFmpegCppPlaybackPreviewOption& option,
                            FFmpegCppPlaybackVideoFrame* frame,
                            FFmpegCppPlaybackPreviewProfile* profile);

private:
    /** 内部checkPreviewCanceled操作，调用方须串行访问此读取器 */
    bool checkPreviewCanceled(const FFmpegCppPlaybackPreviewOption& option, FFmpegCppPlaybackPreviewProfile* profile);

    /** 内部fillPreviewVideoFrameInfo操作，调用方须串行访问此读取器 */
    bool fillPreviewVideoFrameInfo(const AVFrame* frame, int64_t* nextVideoTimestamp100ns, FFmpegCppPlaybackVideoFrame* frameInfo);

    /** 内部receivePreviewFrames操作，调用方须串行访问此读取器 */
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
                              FFmpegCppPlaybackPreviewProfile* profile);

    /** 内部convertVideoFrameToBgra操作，调用方须串行访问此读取器 */
    bool convertVideoFrameToBgra(const AVFrame* frame, FFmpegCppPlaybackVideoFrame* playbackFrame);

    /** 内部openDecoder操作，调用方须串行访问此读取器 */
    bool openDecoder(int streamIndex, bool video);

    /** 内部openAudioResampler操作，调用方须串行访问此读取器 */
    bool openAudioResampler();

    /** 内部fillMediaInfo操作，调用方须串行访问此读取器 */
    void fillMediaInfo();

    /** 内部resetReadState操作，调用方须串行访问此读取器 */
    void resetReadState(int64_t position100ns);

    /** 内部popQueuedFrame操作，调用方须串行访问此读取器 */
    bool popQueuedFrame(FFmpegCppPlaybackFrame* frame);

    /** 内部drainOneDecoder操作，调用方须串行访问此读取器 */
    bool drainOneDecoder();

    /** 内部decodePacket操作，调用方须串行访问此读取器 */
    bool decodePacket(AVCodecContext* codecContext, AVPacket* packet, bool video);

    /** 内部receiveFrames操作，调用方须串行访问此读取器 */
    bool receiveFrames(AVCodecContext* codecContext, bool video);

    /** 内部queueVideoFrame操作，调用方须串行访问此读取器 */
    bool queueVideoFrame(const AVFrame* frame);

    /** 内部queueAudioFrame操作，调用方须串行访问此读取器 */
    bool queueAudioFrame(const AVFrame* frame);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_filePath;
    std::string m_lastError;
    FFmpegCppMediaInfo m_mediaInfo;
    FFmpegCppPlaybackOpenOption m_option;
    std::deque<FFmpegCppPlaybackFrame> m_frameQueue;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    FFmpegCppAdjacentReader m_adjacent;
    AVFormatContext* m_formatContext;
    AVCodecContext* m_videoCodecContext;
    AVCodecContext* m_audioCodecContext;
    SwsContext* m_scaleContext;
    SwrContext* m_resampleContext;
    AVPacket* m_packet;
    AVFrame* m_frame;
    int m_videoStreamIndex;
    int m_audioStreamIndex;
    AVRational m_videoTimeBase;
    AVRational m_audioTimeBase;
    int64_t m_videoStartTime100ns;
    int64_t m_audioStartTime100ns;
    int64_t m_duration100ns;
    int64_t m_videoFrameDuration100ns;
    int64_t m_nextVideoTimestamp100ns;
    int64_t m_nextAudioTimestamp100ns;
    int32_t m_videoWidth;
    int32_t m_videoHeight;
    int32_t m_audioSampleRate;
    int32_t m_audioChannels;
    int32_t m_audioBitsPerSample;
    bool m_isOpen;
    bool m_drainStarted;
    bool m_videoDrained;
    bool m_audioDrained;
    uint64_t m_readPacketCount;
    uint64_t m_videoFrameCount;
    uint64_t m_audioFrameCount;
};