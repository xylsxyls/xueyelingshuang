#include "FFmpegCppTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstring>
#if defined(_WIN32) && defined(FFMPEGCPP_WINDOWS_ANSI_PATH)
#include <cwchar>
#include <windows.h>
#endif
#include "FFmpegCpp/FFmpegCppAPI.h"
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#ifndef __EMSCRIPTEN__
#include <thread>
#endif
#include <iostream>
#include <locale>

#ifdef _WIN32
#include "FFmpegCpp/FFmpegCppNativeMediaWriter.h"
#include <type_traits>

// VS2013的is_copy_assignable会把具有私有deleted赋值的类报告为可赋值。
// 本版用编译器的平凡赋值特征捕获隐式浅复制；较新工具链直接检查公开可赋值性。
#if defined(_MSC_VER) && _MSC_VER == 1800
template<class T> using FFmpegCppTestCopyAssignment = std::is_trivially_copy_assignable<T>;
#else
template<class T> using FFmpegCppTestCopyAssignment = std::is_copy_assignable<T>;
#endif
static_assert(!std::is_copy_constructible<FFmpegCppComPtr<IMFSample>>::value &&
    !FFmpegCppTestCopyAssignment<FFmpegCppComPtr<IMFSample>>::value, "COM reference guard must own a single reference");
static_assert(!std::is_copy_constructible<FFmpegCppNativeMediaWriterState>::value &&
    !FFmpegCppTestCopyAssignment<FFmpegCppNativeMediaWriterState>::value, "MF session lifetime must be unique");
#endif
#include <sstream>

std::string FFmpegCppTestGrouping::do_grouping() const
{
    return "\3";
}

/** 检查测试输出能否打开且包含数据
@param [in] filePath 文件路径，调用期间借用
@return true表示文件长度大于0
*/
static bool ffmpegCppTestFileExistsAndNotEmpty(const std::string& filePath)
{
    std::ifstream stream(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!stream.good())
    {
        return false;
    }
    return stream.tellg() > 0;
}

/** 检查测试图片是否具有JPEG起止标记
@param [in] imageData 待检查的编码图片数据
@return true表示首尾标记匹配，不表示完成了图片解码校验
*/
static bool ffmpegCppTestIsJpeg(const FFmpegCppImageData& imageData)
{
    if (imageData.data.size() < 4)
    {
        return false;
    }
    return imageData.data[0] == 0xFF
        && imageData.data[1] == 0xD8
        && imageData.data[imageData.data.size() - 2] == 0xFF
        && imageData.data[imageData.data.size() - 1] == 0xD9;
}

/** 创建16×16渐变色MJPEG测试视频。像素图由本工程生成。
@param [in] filePath 本次测试的输出路径
@param [in] frameCount 写入的帧数，普通用例默认三帧
@return 完整写入返回true
*/
static bool ffmpegCppTestWriteSample(const std::string& filePath, int32_t frameCount = 3)
{
    static const unsigned char frame[] =
    {
        0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01,
        0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x08, 0x06, 0x06,
        0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09, 0x09, 0x08, 0x0a, 0x0c, 0x14, 0x0d,
        0x0c, 0x0b, 0x0b, 0x0c, 0x19, 0x12, 0x13, 0x0f, 0x14, 0x1d, 0x1a, 0x1f, 0x1e, 0x1d,
        0x1a, 0x1c, 0x1c, 0x20, 0x24, 0x2e, 0x27, 0x20, 0x22, 0x2c, 0x23, 0x1c, 0x1c, 0x28,
        0x37, 0x29, 0x2c, 0x30, 0x31, 0x34, 0x34, 0x34, 0x1f, 0x27, 0x39, 0x3d, 0x38, 0x32,
        0x3c, 0x2e, 0x33, 0x34, 0x32, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x09, 0x09, 0x09, 0x0c,
        0x0b, 0x0c, 0x18, 0x0d, 0x0d, 0x18, 0x32, 0x21, 0x1c, 0x21, 0x32, 0x32, 0x32, 0x32,
        0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
        0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
        0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32, 0x32,
        0x32, 0x32, 0x32, 0x32, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x00, 0x10, 0x00, 0x10, 0x03,
        0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xc4, 0x00, 0x1f, 0x00,
        0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
        0xff, 0xc4, 0x00, 0xb5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05,
        0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05,
        0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81,
        0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62,
        0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29,
        0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
        0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
        0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84,
        0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
        0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5,
        0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca,
        0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5,
        0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
        0xfa, 0xff, 0xc4, 0x00, 0x1f, 0x01, 0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
        0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x11, 0x00, 0x02, 0x01,
        0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00,
        0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
        0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23,
        0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1,
        0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75,
        0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a,
        0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6,
        0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
        0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3,
        0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xff, 0xda, 0x00, 0x0c, 0x03, 0x01, 0x00,
        0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00, 0xf3, 0xfb, 0x1d, 0x0f, 0xa7, 0xc9, 0x5d,
        0x35, 0x8e, 0x87, 0xd3, 0xe4, 0xae, 0x96, 0xc7, 0x43, 0xe9, 0xf2, 0x57, 0x4f, 0x63,
        0xa1, 0xf4, 0xf9, 0x2b, 0x4c, 0x5e, 0x6b, 0xe6, 0x2e, 0x1f, 0xce, 0xf6, 0xd4, 0xff,
        0xd9,
    };
    std::ofstream output(filePath.c_str(), std::ios::binary);
    for (int index = 0; index < frameCount && output; ++index)
    {
        output.write(reinterpret_cast<const char*>(frame), sizeof(frame));
    }
    output.close();
    return output.good();
}

/** 使用公开接口运行固定输入的性能测试
@return 正确性校验通过为0，失败返回非0
*/
static int RunBenchmarks()
{
    const size_t frames = 256;
    const size_t blocks = 256;
    std::vector<int16_t> input(frames * 2, 1500), output(4096 * 2);
    // 一次操作跑完整双声道PCM流，包括组件初始化、分块转换、排空和释放
    LibraryBenchmark::run("soundtouch.stereo48k.256x256_frames", [&](uint32_t count) -> uint64_t
    {
        uint64_t total = 0;
        for (uint32_t round = 0; round < count; ++round)
        {
            FFmpegCppAudioTempo tempo;
            if (!tempo.init(48000, 2) || !tempo.setRate(1000) || !tempo.setPitchSemitones(0))
            {
                throw std::runtime_error("tempo init failed");
            }
            size_t received = 0;
            for (size_t i = 0; i < blocks; ++i)
            {
                if (!tempo.write(input.data(), frames))
                {
                    throw std::runtime_error("tempo input failed");
                }
                received += tempo.read(output.data(), 4096);
            }
            if (!tempo.finish())
            {
                throw std::runtime_error("tempo flush failed");
            }
            while (tempo.available() != 0)
            {
                const size_t read = tempo.read(output.data(), 4096);
                if (read == 0)
                {
                    throw std::runtime_error("tempo stopped draining");
                }
                received += read;
            }
            if (received != frames * blocks)
            {
                throw std::runtime_error("incorrect output frame count");
            }
            total += received;
        }
        return total;
    });
    std::ostringstream filename;
    filename << "ffmpeg-benchmark-" << std::chrono::high_resolution_clock::now().time_since_epoch().count()
        << '-' << &input << ".mjpeg";
    const std::string path = filename.str();
    if (!ffmpegCppTestWriteSample(path))
    {
        std::remove(path.c_str());
        return 1;
    }
    bool success = true;
    try
    {
        // 模拟用户生成缩略图的完整调用，每次重新打开媒体、解码首帧并编码JPEG
        LibraryBenchmark::run("ffmpeg.open_thumbnail.mjpeg16", [&](uint32_t count) -> uint64_t
        {
            uint64_t bytes = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                FFmpegCpp media;
                FFmpegCppImageData thumbnail;
                if (!media.open(path) || !media.extractFirstFrameToMemory(&thumbnail) ||
                    !ffmpegCppTestIsJpeg(thumbnail))
                {
                    throw std::runtime_error("thumbnail generation failed");
                }
                bytes += thumbnail.data.size();
            }
            return bytes;
        });
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "%s\n", error.what());
        success = false;
    }
    return std::remove(path.c_str()) == 0 && success ? 0 : 1;
}

/** 验证实时显示边界在设置、重新定位和打开文件时的行为
@param [in] path 0、40、80 毫秒分别显示黑、白、灰帧的 MP4
@param [in] check 登记时间和像素的独立判定
*/
static void CheckPresentationFloor(const std::string& path,
    const std::function<void(bool, const std::string&)>& check)
{
    FFmpegCppPlaybackReader reader;
    FFmpegCppPlaybackOpenOption option;
    option.decodeAudio = false;
    const auto reads = [&](int64_t start, int level) -> bool
    {
        FFmpegCppPlaybackFrame frame;
        return reader.read(&frame) == FFmpegCppPlaybackReadResultFrame &&
            frame.type == FFmpegCppPlaybackFrameTypeVideo && frame.videoFrame.timestamp100ns == start &&
            !frame.videoFrame.bgraPixels.empty() && std::abs(static_cast<int>(frame.videoFrame.bgraPixels[0]) - level) < 16;
    };
    check(reader.open(path, option) && reads(0, 0), "default display floor delivers first black frame");
    reader.setVideoPresentationFloor(800000);
    check(reads(800000, 96), "raised display floor drops expired white frame before delivery");
    check(reader.seek(0) && reads(0, 0), "seek resets display floor to deliver black frame");
    reader.setVideoPresentationFloor(800000);
    check(reader.open(path, option) && reads(0, 0), "reopen resets display floor");
    reader.setVideoPresentationFloor(400001);
    check(reads(400000, 255), "display floor keeps white frame covering requested time");
    FFmpegCppPlaybackPreviewOption preview;
    FFmpegCppPlaybackVideoFrame first;
    check(reader.seekVideoFrame(0, preview, &first, nullptr) && first.timestamp100ns == 0,
        "exact video seek retains following frame for sequential read");
    reader.setVideoPresentationFloor(800000);
    check(reads(800000, 96), "new display floor applies to buffered frame after exact seek");
    const int64_t disabled[] = { 0, -1 };
    for (size_t i = 0; i < 2; ++i)
    {
        const bool rewind = reader.seek(0);
        reader.setVideoPresentationFloor(800000);
        reader.setVideoPresentationFloor(disabled[i]);
        check(rewind && reads(0, 0), "zero or negative floor restores first frame");
    }
}

/** 检查精确时长的单声道 WAV 在重采样、排空和重新定位后的 PCM
@param [in] path 本轮专用临时文件，结束时删除
@param [in] check 将结果登记到当前用例
*/
static void CheckPlaybackResampling(const std::string& path,
    const std::function<void(bool, const std::string&)>& check)
{
    // 100 ms 的 48 kHz 单声道 8 位无符号 PCM，所有输入样本均为 192。
    const unsigned char header[] = {
        'R','I','F','F',0xe4,0x12,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
        1,0,1,0,0x80,0xbb,0,0,0x80,0xbb,0,0,1,0,8,0,'d','a','t','a',0xc0,0x12,0,0
    };
    std::ofstream wave(path.c_str(), std::ios::binary);
    wave.write(reinterpret_cast<const char*>(header), sizeof(header));
    wave.write(std::string(4800, '\xc0').data(), 4800);
    wave.close();
    check(wave.good(), "create 100 ms PCM fixture");
    FFmpegCppPlaybackReader reader;
    FFmpegCppPlaybackOpenOption option;
    option.decodeVideo = false;
    option.outputSampleRate = 44100;
    option.outputChannels = 1;
    check(reader.open(path, option), "open 48 kHz mono source for 44.1 kHz output");
    for (int pass = 0; pass < 2; ++pass)
    {
        if (pass != 0)
        {
            check(reader.seek(0), "seek after resampler drain");
        }
        size_t samples = 0;
        bool valid = true;
        int16_t last = 0;
        FFmpegCppPlaybackFrame frame;
        FFmpegCppPlaybackReadResult status;
        while ((status = reader.read(&frame)) == FFmpegCppPlaybackReadResultFrame && samples <= 4410)
        {
            const FFmpegCppPlaybackAudioFrame& audio = frame.audioFrame;
            const int64_t expected = static_cast<int64_t>(samples) * 10000000 / 44100;
            if (frame.type != FFmpegCppPlaybackFrameTypeAudio || audio.channels != 1 ||
                audio.sampleRate != 44100 || audio.bitsPerSample != 16 || audio.pcmData.empty() ||
                audio.pcmData.size() % sizeof(last) != 0)
            {
                valid = false;
                break;
            }
            valid = valid && audio.timestamp100ns >= expected - 227 && audio.timestamp100ns <= expected + 227;
            std::memcpy(&last, &audio.pcmData[audio.pcmData.size() - sizeof(last)], sizeof(last));
            samples += audio.pcmData.size() / sizeof(last);
        }
        check(samples == 4410 && last >= 16380 && last <= 16388, "resampled sample count and terminal waveform");
        check(valid, "continuous resampled PCM timestamps and format");
        check(status == FFmpegCppPlaybackReadResultEnd && reader.read(&frame) == FFmpegCppPlaybackReadResultEnd,
            "resampler drain returns EOF exactly once without duplicate output");
    }
    reader.close();
    check(std::remove(path.c_str()) == 0, "remove PCM fixture");
}

/** 使用内置三帧 MJPEG 验证帮助函数、磁盘输出和转封装的取消通知
@param [in] input 本轮写入的素材
@param [in] stem 本轮临时文件名前缀
@param [in] media 已成功打开该素材的媒体对象
@param [in] check 记录当前断言和名称
*/
static void CheckMediaOperations(const std::string& input, const std::string& stem, FFmpegCpp& media,
    const std::function<void(bool, const std::string&)>& check)
{
    FFmpegCppMediaInfo info;
    FFmpegCppStreamInfo audio;
    std::string error;
    // 每行两个 BGRA 像素和四个填充字节。目标为三列时应复制像素并将末列清零
    const unsigned char padded[] = {
        1, 2, 3, 255, 4, 5, 6, 255, 90, 91, 92, 93,
        7, 8, 9, 255, 10, 11, 12, 255, 94, 95, 96, 97
    };
    const unsigned char expected[] = {
        1, 2, 3, 255, 4, 5, 6, 255, 0, 0, 0, 0,
        7, 8, 9, 255, 10, 11, 12, 255, 0, 0, 0, 0
    };
    const std::vector<unsigned char> source(padded, padded + sizeof(padded));
    const std::vector<unsigned char> canvas(expected, expected + sizeof(expected));
    std::vector<unsigned char> fitted;
    check(FFmpegCppHelper::fitBgraFrame(source, 2, 2, 12, 3, 2, &fitted) && fitted == canvas,
        "fit BGRA canvas ignores source padding and clears right edge");
    fitted = source;
    check(FFmpegCppHelper::fitBgraFrame(fitted, 2, 2, 12, 3, 2, &fitted) && fitted == canvas,
        "fit BGRA canvas supports aliased source and destination");
    fitted = canvas;
    check(!FFmpegCppHelper::fitBgraFrame(fitted, 2, 2, 7, 3, 2, &fitted) && fitted == canvas,
        "short BGRA stride preserves destination");
    // 常量颜色缩小后保持每个通道的值，覆盖同一缓冲区缩容的路径
    const std::vector<unsigned char> solid(8 * 8 * 4, 89);
    fitted = solid;
    check(FFmpegCppHelper::fitBgraFrame(fitted, 8, 8, 32, 2, 2, &fitted) &&
        fitted == std::vector<unsigned char>(16, 89), "in-place BGRA downscale retains constant color");
    const std::string suffix("\0unused", 7);
    FFmpegCpp invalidInput;
    FFmpegCppPlaybackReader invalidPlayback;
    check(!invalidInput.open(input + suffix) && !invalidInput.isOpen(), "input with embedded NUL is rejected");
    check(!invalidPlayback.open(input + suffix) && !invalidPlayback.isOpen(), "playback path with embedded NUL is rejected");
    check(!FFmpegCppHelper::probeMediaInfo("", &info, &error) && !error.empty(), "helper rejects empty path");
    check(!FFmpegCppHelper::probeMediaInfo(input, nullptr, &error) && !error.empty(), "helper rejects null info");
    check(FFmpegCppHelper::probeMediaInfo(input, &info, &error) && info.hasVideo && !info.hasAudio &&
        !media.getMainAudioStreamInfo(&audio), "helper probes video-only media");
    check(FFmpegCppHelper::videoCodecToEncoderName(FFmpegCppVideoCodecCopy) == "copy" &&
        FFmpegCppHelper::imageFormatToMimeType(FFmpegCppImageFormatJpeg) == "image/jpeg", "copy and JPEG names");

    FFmpegCppFrameExtractOption imageOption;
    imageOption.maxWidth = 8;
    imageOption.maxHeight = 8;
    FFmpegCppImageData image;
    check(FFmpegCppHelper::extractFirstFrameToMemory(input, &image, imageOption, &error) &&
        ffmpegCppTestIsJpeg(image) && image.width == 8 && image.height == 8, "helper scales JPEG thumbnail");
#if defined(_WIN32) && defined(FFMPEGCPP_WINDOWS_ANSI_PATH)
    // 私有 ACP 构建须使用系统代码页可无损往返的文件名，并按相同宽路径清理。
    const wchar_t* suffixes[] = {L" \u9884\u89c8.jpg", L" vid\u00e9o.jpg", L" preview.jpg"};
    std::wstring wideImage(stem.begin(), stem.end());
    std::string unicodeImage;
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); ++i)
    {
        char encoded[64] = {};
        wchar_t restored[32] = {};
        if (WideCharToMultiByte(CP_ACP, 0, suffixes[i], -1, encoded, sizeof(encoded), nullptr, nullptr) > 0 &&
            MultiByteToWideChar(CP_ACP, 0, encoded, -1, restored, 32) > 0 && std::wcscmp(suffixes[i], restored) == 0)
        {
            unicodeImage = stem + encoded;
            wideImage += suffixes[i];
            break;
        }
    }
#else
    const std::string unicodeImage = stem + " \xe9\xa2\x84\xe8\xa7\x88.jpg";
#endif
    FFmpegCpp reopened;
    FFmpegCppMediaInfo unicodeInfo;
    std::string unicodeSaved;
    check(media.extractFirstFrameToFile(unicodeImage, imageOption, &unicodeSaved) &&
        unicodeSaved == unicodeImage && reopened.open(unicodeSaved) && reopened.getMediaInfo(&unicodeInfo) &&
        unicodeInfo.hasVideo && unicodeInfo.fileSizeBytes > 0, "screenshot path can be reopened with a physical size");
    reopened.close();
#ifdef _WIN32
#ifndef FFMPEGCPP_WINDOWS_ANSI_PATH
    std::wstring wideImage(stem.begin(), stem.end());
    wideImage += L" \u9884\u89c8.jpg";
#endif
    check(_wremove(wideImage.c_str()) == 0, "remove screenshot by its wide path");
#else
    check(std::remove(unicodeImage.c_str()) == 0, "remove UTF-8 screenshot");
#endif
    imageOption.imageFormat = FFmpegCppImageFormatBmp;
    const std::string bmp = stem + ".bmp";
    std::string saved;
    const bool savedBmp = media.extractFrameToFile(0, bmp, imageOption, &saved);
    char magic[2] = {};
    {
        std::ifstream file(bmp.c_str(), std::ios::binary);
        file.read(magic, sizeof(magic));
    }
    check(savedBmp && saved == bmp && magic[0] == 'B' && magic[1] == 'M', "BMP path and bytes on disk");
    check(std::remove(bmp.c_str()) == 0, "remove generated BMP");
    imageOption.imageFormat = FFmpegCppImageFormatPng;
    const unsigned char png[] = {0x89, 'P', 'N', 'G', 13, 10, 26, 10};
    const bool pngEncoded = media.extractFirstFrameToMemory(&image, imageOption);
    check(pngEncoded ? image.data.size() >= sizeof(png) && std::memcmp(image.data.data(), png, sizeof(png)) == 0
        : media.lastError() == "image encoder is not enabled", "PNG matches configured encoder");

    FFmpegCppTranscodeOption copy;
    copy.outputContainer = "avi";
    copy.removeAudio = true;
    copy.audioCodec = FFmpegCppAudioCodecNone;
    copy.videoCodec = FFmpegCppVideoCodecCopy;
    copy.overwriteOutput = true;
    const std::string alias = "./" + input;
    check(!media.transcode(alias, copy) && !media.lastError().empty(), "remux preserves input path alias");
    std::string ignoredPath("untouched");
    check(!media.extractFirstFrameToFile(alias, imageOption, &ignoredPath) && ignoredPath == "untouched",
        "screenshot preserves input path alias and output parameter");
    check(!media.transcode("", copy) && !media.lastError().empty(), "empty transcode destination");
    const std::string guard = stem + "-rejected.avi";
    const std::string invalidPath = guard + suffix;
    imageOption.imageFormat = FFmpegCppImageFormatBmp;
    FFmpegCppMediaWriter invalidWriter;
    check(!media.extractFirstFrameToFile(invalidPath, imageOption, &ignoredPath) && ignoredPath == "untouched",
        "screenshot rejects embedded NUL without publishing path");
    check(!media.transcode(invalidPath, copy), "remux rejects embedded NUL");
    check(!invalidWriter.open(invalidPath, false, 0, 0), "writer rejects embedded NUL");
    check(!std::ifstream(guard.c_str(), std::ios::binary).good(), "output path prefix is untouched");
    const std::string avi = stem + ".avi";
    FFmpegCpp streamCopy;
    check(FFmpegCppHelper::transcodeVideo(input, avi, copy, nullptr, nullptr, &error) &&
        streamCopy.open(avi) && streamCopy.hasVideo() && !streamCopy.hasAudio(), "copy to readable video-only AVI");
    FFmpegCppImageData between;
    check(streamCopy.extractFrameToMemory(30, &between) && between.timestampMilliseconds == 0,
        "30 ms remains in the first frame of the 25 fps AVI");
    streamCopy.close();
    check(std::remove(avi.c_str()) == 0, "remove generated AVI");

    if (!FFmpegCppHelper::isVideoEncoderAvailable(FFmpegCppVideoCodecH265))
    {
        FFmpegCppTranscodeOption encode;
        encode.outputContainer = "mp4";
        encode.videoCodec = FFmpegCppVideoCodecH265;
        encode.audioCodec = FFmpegCppAudioCodecNone;
        encode.removeAudio = true;
        const std::string output = stem + "-unavailable.mp4";
        check(!media.transcode(output, encode) && media.lastError() == "requested video encoder is not enabled",
            "unavailable H265 encoder reports error");
        if (std::ifstream(output.c_str(), std::ios::binary).good())
        {
            check(std::remove(output.c_str()) == 0, "remove unavailable encoder output");
        }
    }

    FFmpegCppTestProgress progress = {false, false};
    /** 第一次数据包通知请求取消，终态通知记录取消标志
    @param [in] event 当前同步进度通知
    @param [in,out] data 指向本用例的进度状态，回调期间有效
    @return false请求停止
    */
    FFmpegCppProgressCallback cancel = [](const FFmpegCppProgress& event, void* data) -> bool
    {
        FFmpegCppTestProgress* state = static_cast<FFmpegCppTestProgress*>(data);
        if (event.cancelled)
        {
            state->cancelled = true;
        }
        else
        {
            state->packet = true;
        }
        return false;
    };
    const std::string partial = stem + "-cancel.avi";
    const bool copied = media.transcode(partial, copy, cancel, &progress);
    check(!copied && progress.packet && progress.cancelled && !media.lastError().empty() &&
        media.cancelCurrentTask(), "cancel stream copy and receive terminal status");
    // 回调返回 true，唯一的取消来源是公开 cancelCurrentTask 接口。
    FFmpegCppProgressCallback explicitCancel = [](const FFmpegCppProgress& event, void* data) -> bool {
        if (!event.cancelled)
        {
            FFmpegCpp* owner = static_cast<FFmpegCpp*>(data);
#ifdef __EMSCRIPTEN__
            owner->cancelCurrentTask();
#else
            std::thread requester(&FFmpegCpp::cancelCurrentTask, owner);
            requester.join();
#endif
        }
        return true;
    };
    check(!media.transcode(partial, copy, explicitCancel, &media) && media.lastError() == "transcode cancelled",
        "public cancellation interrupts the active remux");
    if (std::ifstream(partial.c_str(), std::ios::binary).good())
    {
        check(std::remove(partial.c_str()) == 0, "remove partial output after cancel");
    }
}

/** 用独立的21帧素材校验打开前设置、立即缩容、独立读者默认值及缓存命中
@param [in] stem 本次测试独占的临时路径前缀
@param [in] check 同步记录断言结果
*/
static void CheckAdjacentCacheLimits(const std::string& stem,
    const std::function<void(bool, const std::string&)>& check)
{
    const std::string input = stem + "-cache.mjpeg";
    const std::string output = stem + "-cache.mp4";
    FFmpegCpp source;
    FFmpegCppTranscodeOption copy;
    copy.outputContainer = "mp4";
    copy.videoCodec = FFmpegCppVideoCodecCopy;
    copy.audioCodec = FFmpegCppAudioCodecCopy;
    const bool ready = ffmpegCppTestWriteSample(input, 21) && source.open(input) && source.transcode(output, copy);
    check(ready, "create 21-frame cache fixture");
    source.close();
    if (ready)
    {
        FFmpegCppPlaybackReader reader;
        FFmpegCppPlaybackPreviewOption option;
        FFmpegCppPlaybackPreviewProfile profile;
        FFmpegCppPlaybackVideoFrame frame;
        // 前两次复用同一对象，其余使用独立读者，分别验证帧数、字节数和默认值。
        for (int32_t mode = 0; mode < 5; ++mode)
        {
            FFmpegCppPlaybackReader fresh;
            FFmpegCppPlaybackReader& active = mode < 2 ? reader : fresh;
            if (mode == 0 || mode == 2 || mode == 3)
            {
                check(active.setAdjacentCacheLimits(-1, -1), "configure unlimited cache before open");
            }
            bool visited = active.open(output);
            int64_t origin = 0;
            int64_t second = 0;
            for (int32_t step = 0; visited && step < 18; ++step)
            {
                visited = active.readAdjacentVideoFrameEx(origin, 1, option, &frame, &profile) &&
                    !frame.empty() && frame.timestamp100ns > origin;
                origin = frame.timestamp100ns;
                if (step == 0)
                {
                    second = origin;
                }
            }
            check(visited, "visit 18 adjacent presentation frames");
            if (mode == 2)
            {
                check(active.setAdjacentCacheLimits(3, -1), "shrink only the frame limit while open");
            }
            if (mode == 3)
            {
                check(active.setAdjacentCacheLimits(-1, 1), "shrink only the byte budget while open");
            }
            check(!active.setAdjacentCacheLimits(2, -1) && !active.setAdjacentCacheLimits(-1, 0) &&
                !active.setAdjacentCacheLimits(-2, -1) && !active.setAdjacentCacheLimits(-1, -2),
                "invalid cache bounds preserve both limits");
            const bool previous = visited && active.readAdjacentVideoFrameEx(second, -1, option, &frame, &profile) &&
                frame.timestamp100ns == 0 && !frame.empty();
            check(previous && (mode < 2 ? profile.decodedVideoFrameCount == 0 : profile.decodedVideoFrameCount > 0),
                "cache hit or re-decode follows the active limits");
            active.close();
        }
    }
    std::remove(input.c_str());
    std::remove(output.c_str());
}

/** 验证PCM格式、速率、音调和多声道输出
@param [in] pcm 4800帧立体声参考输入
@param [in] check 同步报告断言结果
*/
static void CheckTempo(const std::vector<int16_t>& pcm,
    const std::function<void(bool, const std::string&)>& check)
{
    FFmpegCppAudioTempo tempo;
    check(!tempo.init(999, 2) && !tempo.setRate(1000), "tempo rejects invalid format");
    check(tempo.init(48000, 2), "open stereo tempo");
    check(!tempo.setRate(49) && tempo.setRate(1000), "tempo rate bounds");
    check(!tempo.setPitchSemitones(8) && tempo.setPitchSemitones(0), "tempo pitch bounds");
    check(!tempo.write(nullptr, 1) && tempo.write(pcm.data(), pcm.size() / 2) && tempo.finish(), "flush stereo PCM");
    const size_t available = tempo.available();
    std::vector<int16_t> processed(pcm.size() + 4096);
    const size_t produced = tempo.read(processed.data(), processed.size() / 2);
    check(produced == available && produced >= 4700 && produced <= 4900, "tempo normal duration");
    check(*std::max_element(processed.begin(), processed.end()) > 3000 && tempo.available() == 0, "tempo waveform and drain");
    tempo.uninit();
    check(!tempo.finish() && tempo.read(nullptr, 1) == 0, "tempo closed state");

    // 440 Hz正弦独立验证初始倍速改变时长、升高七个半音改变频率
    std::vector<int16_t> tone(48000);
    for (size_t i = 0; i < tone.size(); ++i)
    {
        tone[i] = static_cast<int16_t>(12000 * std::sin(i * 440.0 * 2.0 * std::acos(-1.0) / 48000.0));
    }
    check(!tempo.init(48000, 1, 49) && !tempo.finish(), "invalid initial tempo leaves stream closed");
    check(!tempo.init(48000, 1, 1000, -8) && !tempo.finish(), "invalid initial pitch leaves stream closed");
    check(tempo.init(48000, 1, 2000, 7) && tempo.write(tone.data(), tone.size()) && tempo.finish(),
        "set initial tempo and pitch before first sample");
    std::vector<int16_t> shifted(tone.size());
    const size_t shiftedFrames = tempo.read(shifted.data(), shifted.size());
    check(shiftedFrames >= 23760 && shiftedFrames <= 24240, "double tempo halves duration");
    size_t crossings = 0;
    if (shiftedFrames >= 22000)
    {
        for (size_t i = 2001; i < 22000; ++i)
        {
            if (shifted[i - 1] <= 0 && shifted[i] > 0)
            {
                ++crossings;
            }
        }
    }
    // 约659 Hz在20000/48000秒内产生约275个正向过零，允许分块边界误差
    check(crossings >= 264 && crossings <= 286, "seven semitones raise tone frequency");

    // 32声道素材每个声道有独立幅值，验证通道顺序和容量上限。
    const int32_t surroundChannels = 32;
    const bool surroundReady = tempo.init(48000, surroundChannels);
    check(surroundReady, "open 32-channel tempo");
    if (surroundReady)
    {
        std::vector<int16_t> surround(4800 * surroundChannels);
        for (size_t i = 0; i < surround.size(); ++i)
        {
            surround[i] = static_cast<int16_t>((i % surroundChannels + 1) * 128);
        }
        check(tempo.write(surround.data(), 4800) && tempo.finish(), "process 32-channel PCM");
        std::vector<int16_t> decoded(surround.size());
        const size_t frames = tempo.read(decoded.data(), 4800);
        bool ordered = frames > 2400;
        for (int32_t channel = 1; ordered && channel < surroundChannels; ++channel)
        {
            ordered = decoded[2400 * surroundChannels + channel] > decoded[2400 * surroundChannels + channel - 1];
        }
        check(ordered, "32-channel PCM preserves channel order");
    }
    check(!tempo.init(48000, surroundChannels + 1), "reject 33-channel PCM");
}

/** 运行独立的媒体测试
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数数组，可通过参数指定测试媒体
@return 全部检查通过返回0，否则返回非0
*/
static int RunSuite(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }

    int32_t totalCount = 0;
    int32_t failCount = 0;

    /** 记录一个测试断言并累计失败数
    @param [in] ok 是否满足该检查的独立预期
    @param [in] name 检查名称，写入控制台
    */
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };

    const bool option = argc == 2 && argv[1][0] == '-';
    if (option && std::strcmp(argv[1], "--list") == 0)
    {
        std::puts("tempo media playback-writer");
        return 0;
    }
    const bool tempoOnly = option && std::strcmp(argv[1], "--case=tempo") == 0;
    const bool mediaOnly = option && std::strcmp(argv[1], "--case=media") == 0;
    const bool playbackOnly = option && std::strcmp(argv[1], "--case=playback-writer") == 0;
    if (argc > 2 || (option && !tempoOnly && !mediaOnly && !playbackOnly))
    {
        return 2;
    }
    const bool all = !option;
    std::vector<int16_t> pcm(4800 * 2);
    for (size_t i = 0; i < pcm.size(); ++i)
    {
        pcm[i] = i / 96 % 2 == 0 ? 6000 : -6000;
    }
    if (tempoOnly)
    {
        CheckTempo(pcm, check);
        return failCount == 0 ? 0 : 1;
    }
    std::ostringstream outputName;
    outputName << "ffmpegcpp_test_" << std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (all || playbackOnly)
    {
        CheckPlaybackResampling(outputName.str() + ".wav", check);
    }
    const bool useFixture = argc <= 1 || option;
    const std::string inputFilePath = useFixture ? outputName.str() + ".mjpeg" : argv[1];
    const std::string outputCopy = outputName.str() + ".mp4";
    if (useFixture)
    {
        check(ffmpegCppTestWriteSample(inputFilePath), "write test video");
    }

    FFmpegCpp emptyObject;
    check(!emptyObject.open(""), "open empty path");
    check(!emptyObject.isOpen(), "empty object not open");
    check(!emptyObject.getMediaInfo(nullptr), "get media info null");
    check(!emptyObject.extractFirstFrameToMemory(nullptr), "extract null image data");
    emptyObject.close();
    emptyObject.close();
    check(!emptyObject.isOpen(), "repeated close");

    check(FFmpegCppHelper::isCommonVideoFileExtension("test.MP4"), "common video extension true");
    check(!FFmpegCppHelper::isCommonVideoFileExtension("test.txt"), "common video extension false");
    check(FFmpegCppHelper::guessVideoMimeTypeByFilePath("a.mkv") == "video/x-matroska", "video mime");
    check(FFmpegCppHelper::imageFormatToExtension(FFmpegCppImageFormatBmp) == "bmp", "image extension bmp");
    check(FFmpegCppHelper::imageFormatToMimeType(FFmpegCppImageFormatPng) == "image/png", "image mime png");
    check(FFmpegCppHelper::videoCodecToEncoderName(FFmpegCppVideoCodecH265) == "libx265", "h265 encoder name");
    check(FFmpegCppHelper::audioCodecToEncoderName(FFmpegCppAudioCodecNone).empty(), "audio none encoder name");
    const std::locale prior = std::locale::global(std::locale(std::locale::classic(), new FFmpegCppTestGrouping));
    const std::string hours = FFmpegCppHelper::formatMilliseconds(3600000000LL);
    std::locale::global(prior);
    check(hours == "1000:00:00.000", "timestamp is independent of global numeric grouping");
    check(FFmpegCppHelper::formatMilliseconds(3723004) == "01:02:03.004", "format milliseconds");

    FFmpegCpp ffmpegCpp;
    check(ffmpegCpp.open(inputFilePath), "open real video");
    check(ffmpegCpp.isOpen() && ffmpegCpp.filePath() == inputFilePath, "open state");

    FFmpegCppMediaInfo mediaInfo;
    check(ffmpegCpp.getMediaInfo(&mediaInfo), "get media info");
    check(mediaInfo.fileSizeBytes > 0 && mediaInfo.hasVideo, "media info values");

    FFmpegCppStreamInfo videoStream;
    check(ffmpegCpp.getMainVideoStreamInfo(&videoStream) && videoStream.width > 0 && videoStream.height > 0, "main video stream");

    FFmpegCppFrameExtractOption extractOption;
    extractOption.maxWidth = 360;
    extractOption.maxHeight = 640;
    extractOption.applyRotation = true;
    extractOption.imageFormat = FFmpegCppImageFormatJpeg;
    extractOption.jpegQuality = 82;
    extractOption.timeoutMilliseconds = 10000;

    FFmpegCppImageData imageData;
    bool extractJpegResult = ffmpegCpp.extractFirstFrameToMemory(&imageData, extractOption);
    if (!extractJpegResult)
    {
        std::cout << "extract jpeg error: " << ffmpegCpp.lastError() << std::endl;
    }
    check(extractJpegResult, "extract first frame jpeg");
    check(!imageData.empty() && imageData.width > 0 && imageData.height > 0 && imageData.mimeType == "image/jpeg", "jpeg image data");
    check(ffmpegCppTestIsJpeg(imageData), "jpeg magic");

    FFmpegCppImageData timedImage;
    check(ffmpegCpp.extractFrameToMemory(0, &timedImage, extractOption) && ffmpegCppTestIsJpeg(timedImage), "decode frame at zero milliseconds");
    check(ffmpegCpp.reloadMediaInfo() && ffmpegCpp.mediaInfo().hasVideo, "reload video metadata");

    FFmpegCppTranscodeOption transcodeOption;
    transcodeOption.videoCodec = FFmpegCppVideoCodecCopy;
    transcodeOption.audioCodec = FFmpegCppAudioCodecCopy;
    transcodeOption.outputContainer = "mp4";
    transcodeOption.overwriteOutput = true;
    transcodeOption.timeoutMilliseconds = 30000;
    bool copyResult = ffmpegCpp.transcode(outputCopy, transcodeOption, nullptr, nullptr);
    if (!copyResult)
    {
        std::cout << "copy transcode error: " << ffmpegCpp.lastError() << std::endl;
    }
    check(copyResult && ffmpegCppTestFileExistsAndNotEmpty(outputCopy), "stream copy transcode");

    if (useFixture && !playbackOnly && ffmpegCpp.isOpen())
    {
        CheckMediaOperations(inputFilePath, outputName.str(), ffmpegCpp, check);
    }

    if (all)
    {
        CheckTempo(pcm, check);
    }

    if (useFixture && copyResult && !mediaOnly)
    {
        CheckAdjacentCacheLimits(outputName.str(), check);
        FFmpegCppPlaybackReader reader;
        FFmpegCppPlaybackFrame frame;
        int32_t width = 0;
        int32_t height = 0;
        check(reader.open(outputCopy) && reader.isOpen(), "playback opens remuxed MP4");
        check(reader.hasVideo() && !reader.hasAudio() && reader.getVideoSize(&width, &height) &&
            width == 16 && height == 16, "playback stream layout");
        size_t frameCount = 0;
        bool pixelsMatch = true;
        FFmpegCppPlaybackReadResult result;
        while ((result = reader.read(&frame)) == FFmpegCppPlaybackReadResultFrame && frameCount < 4)
        {
            pixelsMatch = pixelsMatch && frame.type == FFmpegCppPlaybackFrameTypeVideo &&
                frame.videoFrame.stride == 64 && frame.videoFrame.bgraPixels.size() == 1024;
            ++frameCount;
        }
        check(frameCount == 3 && pixelsMatch && result == FFmpegCppPlaybackReadResultEnd, "three BGRA frames then EOF");
        check(reader.seek(0) && reader.read(&frame) == FFmpegCppPlaybackReadResultFrame &&
            frame.videoFrame.timestamp100ns == 0, "seek back to first frame");
        FFmpegCppPlaybackVideoFrame preview;
        check(reader.readVideoFrameAt(0, 8, &preview) && !preview.empty(), "random access preview");
        FFmpegCppPlaybackPreviewOption previewOption;
        FFmpegCppPlaybackPreviewProfile profile;
        FFmpegCppPlaybackVideoFrame adjacent;
        check(previewOption.frameBudget() == 8192, "default preview decode budget");
        previewOption.maxReadFrameCount = 1;
        check(!reader.readAdjacentVideoFrameEx(0, 1, previewOption, &adjacent, &profile) &&
            profile.decodedVideoFrameCount == 1, "adjacent preview stops after one decoded frame");
        previewOption.maxReadFrameCount = static_cast<size_t>(-1);
        check(previewOption.frameBudget() == static_cast<size_t>(-1), "unlimited preview decode budget");
        check(reader.readAdjacentVideoFrameEx(0, 1, previewOption, &adjacent, &profile) &&
            adjacent.timestamp100ns > 0 && !adjacent.empty(), "decode next presentation frame");
        const int64_t secondStart = adjacent.timestamp100ns;
        check(reader.readAdjacentVideoFrameEx(secondStart, -1, previewOption, &adjacent, &profile) &&
            adjacent.timestamp100ns == 0, "decode previous presentation frame");
        check(!reader.readAdjacentVideoFrameEx(-1, 1, previewOption, &adjacent, &profile) &&
            adjacent.empty() && !reader.lastError().empty(), "invalid adjacent origin clears old pixels and reports error");
        // 已缓存的同一目标执行路径一致；只在最后一次回调取消，验证转换后的像素也被丢弃。
        size_t calls = 0;
        previewOption.cancelUserData = &calls;
        previewOption.cancelCallback = [](void* data) -> bool { ++*static_cast<size_t*>(data); return false; };
        check(reader.readAdjacentVideoFrameEx(secondStart, -1, previewOption, &adjacent, &profile) &&
            calls > 0 && !adjacent.empty(), "measure cancellation checks on cached adjacent frame");
        previewOption.cancelCallback = [](void* data) -> bool { return --*static_cast<size_t*>(data) == 0; };
        check(!reader.readAdjacentVideoFrameEx(secondStart, -1, previewOption, &adjacent, &profile) &&
            profile.canceled && adjacent.empty() && !reader.lastError().empty(), "late adjacent cancellation publishes no pixels");
        previewOption.cancelCallback = nullptr;
        previewOption.cancelUserData = nullptr;
        check(reader.readVideoFrameInfoAt(0, previewOption, &adjacent, &profile) &&
            adjacent.timestamp100ns == 0 && adjacent.bgraPixels.empty(), "metadata-only preview omits pixels");
        previewOption.cancelCallback = [](void*) -> bool { return true; };
        check(!reader.readVideoFrameAtEx(0, previewOption, &adjacent, &profile) && profile.canceled,
            "cancel preview before decoding");
        previewOption.cancelCallback = nullptr;
        check(reader.readAdjacentVideoFrameEx(0, 1, previewOption, &adjacent, &profile) && !adjacent.empty(),
            "valid adjacent request after cancellation");
        reader.close();
        check(!reader.readAdjacentVideoFrameEx(0, 1, previewOption, &adjacent, &profile) &&
            adjacent.empty() && !reader.lastError().empty(), "closed adjacent read clears previously returned pixels");
        check(!reader.isOpen() && reader.read(&frame) == FFmpegCppPlaybackReadResultError, "closed reader error");
        const std::string encodedPath = outputName.str() + "_encoded.mp4";
        FFmpegCppMediaWriter writer;
        check(!writer.open("", true, 16, 16), "writer rejects empty path");
        if (FFmpegCppHelper::isVideoEncoderAvailable(FFmpegCppVideoCodecH264))
        {
            // 无法创建的目录使打开失败；下一次使用同一写入器应能成功
            check(!writer.open(outputName.str() + "/missing/output.mp4", true, 16, 16) &&
                !writer.error().empty(), "writer releases partial open after destination failure");
            const bool opened = writer.open(encodedPath, true, 16, 16);
            check(opened, "writer creates H264/AAC MP4");
            if (opened)
            {
                check(!writer.video(nullptr, 0, 0), "writer rejects empty video frame");
                std::vector<unsigned char> color(16 * 16 * 4, 0);
                check(writer.video(color.data(), color.size(), 0, 400000), "write black frame at zero");
                std::fill(color.begin(), color.end(), 255);
                check(writer.video(color.data(), color.size(), 400000, 400000), "write white frame at 40 ms");
                std::fill(color.begin(), color.end(), 96);
                check(writer.video(color.data(), color.size(), 800000, 400000), "write grey frame at 80 ms");
                check(writer.audio(pcm.data(), pcm.size() / 2), "write stereo audio");
                check(!writer.open(encodedPath, true, 16, 16), "reopening active writer preserves active output");
                check(writer.finish(), "finalize MP4");
                CheckPresentationFloor(encodedPath, check);
                check(!writer.finish(), "finish on closed writer fails");
                const std::string nextPath = outputName.str() + "_next.mp4";
                const bool reopenedWriter = writer.open(nextPath, true, 16, 16);
                check(reopenedWriter, "same writer opens another file after finish");
                if (reopenedWriter)
                {
                    check(writer.video(preview.bgraPixels.data(), preview.bgraPixels.size(), 0) &&
                        writer.audio(pcm.data(), pcm.size() / 2) && writer.finish(), "new recording starts at zero and finalizes");
                    FFmpegCpp secondRecording;
                    check(secondRecording.open(nextPath) && secondRecording.hasVideo() && secondRecording.hasAudio(),
                        "second recording contains fresh video and audio streams");
                    secondRecording.close();
                    check(std::remove(nextPath.c_str()) == 0, "remove second generated recording");
                }
                FFmpegCpp encoded;
                FFmpegCppMediaInfo encodedInfo;
                check(encoded.open(encodedPath) && encoded.getMediaInfo(&encodedInfo) &&
                    encodedInfo.hasVideo && encodedInfo.hasAudio, "reopen encoded video and audio streams");
                FFmpegCppFrameExtractOption bitmap;
                bitmap.imageFormat = FFmpegCppImageFormatBmp;
                const int times[] = {45, 1, 80};
                const int starts[] = {40, 0, 80};
                const int brightness[] = {255, 0, 96};
                for (size_t index = 0; index < 3; ++index)
                {
                    FFmpegCppImageData sample;
                    const bool extracted = encoded.extractFrameToMemory(times[index], &sample, bitmap);
                    size_t pixel = 0;
                    if (sample.data.size() >= 54)
                    {
                        for (int i = 3; i >= 0; --i)
                        {
                            pixel = pixel * 256 + sample.data[10 + i];
                        }
                    }
                    const bool matches = extracted && sample.timestampMilliseconds == starts[index] && pixel >= 54 &&
                        pixel < sample.data.size() && std::abs(static_cast<int>(sample.data[pixel]) - brightness[index]) < 16;
                    if (!matches)
                    {
                        std::cerr << "snapshot request=" << times[index] << " expected pts=" << starts[index]
                            << " actual pts=" << sample.timestampMilliseconds << " pixel="
                            << (pixel < sample.data.size() ? static_cast<int>(sample.data[pixel]) : -1)
                            << " error=" << encoded.lastError() << std::endl;
                    }
                    check(matches,
                        "snapshot contains the displayed pixel and its real presentation timestamp");
                }
            }
            if (std::ifstream(encodedPath.c_str(), std::ios::binary).good())
            {
                check(std::remove(encodedPath.c_str()) == 0, "remove generated H264/AAC file");
            }
        }
    }

    std::cout << "h264 encoder available=" << (FFmpegCppHelper::isVideoEncoderAvailable(FFmpegCppVideoCodecH264) ? "true" : "false") << std::endl;
    std::cout << "h265 encoder available=" << (FFmpegCppHelper::isVideoEncoderAvailable(FFmpegCppVideoCodecH265) ? "true" : "false") << std::endl;

    ffmpegCpp.close();
    check(!ffmpegCpp.isOpen(), "close after work");
    if (copyResult || std::ifstream(outputCopy.c_str(), std::ios::binary).good())
    {
        check(std::remove(outputCopy.c_str()) == 0, "remove encoded test output");
    }
    if (useFixture)
    {
        check(std::remove(inputFilePath.c_str()) == 0, "remove generated video");
    }

    std::cout << "FFmpegCpp runtime test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

/** 执行所选功能组，默认运行完整套件。
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数数组
@return 0 表示通过，1 表示断言失败，2 表示参数无效
*/
int main(int argc, char* argv[])
{
    const LibraryNamedTestCase cases[] =
    {
        {FFmpegCppTestTempo, false, "tempo"},
        {FFmpegCppTestMedia, false, "media"},
        {FFmpegCppTestPlaybackWriter, false, "playback-writer"},
    };
    return LibraryCaseSelection::runNamed(argc, argv, cases, RunSuite);
}