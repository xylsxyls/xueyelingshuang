#pragma once
#include "FFmpegCppMacro.h"
#include <stdint.h>
#include <string>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct SwrContext;
struct SwsContext;
struct AVFrame;
class FFmpegCppNativeMediaWriter;

// Sequential 48 kHz stereo S16 audio and top-down BGRA video encoder.
class FFmpegCppAPI FFmpegCppMediaWriter
{
public:
    FFmpegCppMediaWriter();
    ~FFmpegCppMediaWriter();
    bool open(const std::string& path, bool video, int width, int height);
    bool audio(const int16_t* samples, size_t frames);
    bool video(const unsigned char* pixels, size_t bytes, int64_t time100ns);
    bool finish();
    std::string error() const;
private:
    FFmpegCppMediaWriter(const FFmpegCppMediaWriter&);
    FFmpegCppMediaWriter& operator=(const FFmpegCppMediaWriter&);
    bool encode(AVCodecContext* codec, AVStream* stream, AVFrame* frame);
    bool audioBlock(size_t frames);
    bool fail(int code, const char* stage);
    void close();
    AVFormatContext* m_format;
    AVCodecContext* m_audio;
    AVCodecContext* m_video;
    AVStream* m_audioStream;
    AVStream* m_videoStream;
    SwrContext* m_resampler;
    SwsContext* m_scaler;
    int64_t m_audioFrames;
    bool m_started;
    FFmpegCppNativeMediaWriter* m_native;
    int m_width, m_height;
    std::vector<int16_t> m_pending;
    std::string m_error;
};