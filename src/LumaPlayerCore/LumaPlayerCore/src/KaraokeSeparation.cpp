#include "KaraokeSeparation.h"
#include "KaraokeSpectrum.h"
#include "KaraokeOrt.h"
#include "KaraokeSeparationTask.h"
#include "LogManager/LogManagerAPI.h"
#include <chrono>
#include "FFmpegCpp/FFmpegCppAPI.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

/** 同步报告分离进度并检查调用方是否取消
@param [in] callback 可为空的同步回调，返回零表示取消
@param [in] user 借用的上下文，在整个分离调用期间有效
@param [in] value 分离进度，0至100
@return true表示继续处理
*/
static bool ReportSeparationProgress(KaraokeSeparationProgress callback, void* user, int value)
{
    return callback == nullptr || callback(value, user) != 0;
}

std::shared_ptr<const KaraokeBacking> KaraokeSeparation::run(const char* source, const char* runtime,
    const char* model, size_t maxDecodedBytes, KaraokeSeparationProgress progress, void* user,
    std::string* error, int windowFrames, int overlapFrames)
{
    const size_t kSampleRate = 44100;
    const size_t kModelWindowFrames = KaraokeSpectrum::Frames;
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    /** Store a normal failure and return no installable backing
    @param [in] message Diagnostic text
    @return Empty result; caller converts diagnostics at the C ABI boundary
    */
    const auto fail = [error](const char* message) -> std::shared_ptr<const KaraokeBacking>
    {
        if (error != nullptr)
        {
            *error = message && *message ? message : "Vocal separation failed without a diagnostic";
        }
        return std::shared_ptr<const KaraokeBacking>();
    };
    if (error != nullptr)
    {
        error->clear();
    }
    if (source == nullptr || runtime == nullptr || model == nullptr ||
        maxDecodedBytes < kModelWindowFrames * 8 || maxDecodedBytes > static_cast<size_t>(512) * 1024 * 1024 ||
        windowFrames != KaraokeSpectrum::Frames || overlapFrames < KaraokeSpectrum::FftSize || overlapFrames >= windowFrames)
    {
        return fail("Invalid MDX settings: window=261120 frames, overlap=5120..261119 frames");
    }

    const size_t chunk = static_cast<size_t>(windowFrames);
    const size_t overlap = static_cast<size_t>(overlapFrames);
    const size_t step = chunk - overlap;
    const size_t border = KaraokeSpectrum::FftSize / 2;
    if (!ReportSeparationProgress(progress, user, 0))
    {
        return fail("Vocal separation canceled");
    }

    FFmpegCppPlaybackReader reader;
    FFmpegCppPlaybackOpenOption option;
    option.decodeVideo = false;
    option.decodeAudio = true;
    option.outputSampleRate = static_cast<int>(kSampleRate);
    option.outputChannels = 2;
    option.outputBitsPerSample = 16;
    if (!reader.open(source, option))
    {
        return fail("Cannot decode source audio");
    }
    std::shared_ptr<KaraokeBacking> backing(new KaraokeBacking);
    backing->m_source = source;
    backing->m_samples.reset(new KaraokeStorage);
    KaraokeStorage mix;
    if (!backing->m_samples->valid() || !mix.valid())
    {
        return fail(!mix.valid() ? mix.error() : backing->m_samples->error());
    }
    FFmpegCppPlaybackFrame frame;
    for (;;)
    {
        if (!ReportSeparationProgress(progress, user, 0))
        {
            return fail("Vocal separation canceled");
        }
        const FFmpegCppPlaybackReadResult result = reader.read(&frame);
        if (result == FFmpegCppPlaybackReadResultEnd)
        {
            break;
        }
        if (result != FFmpegCppPlaybackReadResultFrame)
        {
            return fail("Source audio decode failed");
        }
        if (frame.type != FFmpegCppPlaybackFrameTypeAudio)
        {
            continue;
        }
        const FFmpegCppPlaybackAudioFrame& audio = frame.audioFrame;
        if (audio.sampleRate != static_cast<int>(kSampleRate) || audio.channels != 2 || audio.pcmData.size() % 4 != 0)
        {
            return fail("Unexpected decoded audio format");
        }
        const int64_t timestamp = (std::max<int64_t>)(0, audio.timestamp100ns);
        if (timestamp > (INT64_MAX - 5000000) / static_cast<int64_t>(kSampleRate))
        {
            return fail("Source timestamp overflow");
        }
        const int64_t start = (timestamp * static_cast<int64_t>(kSampleRate) + 5000000) / 10000000;
        const size_t count = audio.pcmData.size() / 2;
        const uint64_t offset = static_cast<uint64_t>(start) * 8;
        while (mix.size() < offset + count * 4)
        {
            if (!ReportSeparationProgress(progress, user, 0))
            {
                return fail("Vocal separation canceled");
            }
            if (mix.append(nullptr, static_cast<size_t>((std::min<uint64_t>)(1024 * 1024, offset + count * 4 - mix.size()))) == UINT64_MAX)
            {
                return fail(mix.error());
            }
        }
        std::vector<float> decoded(count);
        for (size_t i = 0; i < count; ++i)
        {
            int16_t value;
            std::memcpy(&value, &audio.pcmData[i * 2], 2);
            decoded[i] = value / 32768.0f;
        }
        if (!mix.write(offset, decoded.data(), count * sizeof(float)))
        {
            return fail(mix.error());
        }
    }
    reader.close();
    if (mix.size() == 0)
    {
        return fail("Source contains no audio");
    }
    const uint64_t frameCount64 = mix.size() / 8;
    if (frameCount64 > static_cast<uint64_t>((std::numeric_limits<size_t>::max)() / 12) ||
        frameCount64 > static_cast<uint64_t>((std::numeric_limits<size_t>::max)() - chunk * 3))
    {
        return fail("Source is too long for this platform's separation indexing");
    }
    const size_t frames = static_cast<size_t>(frameCount64);
    if (!ReportSeparationProgress(progress, user, 5))
    {
        return fail("Vocal separation canceled");
    }

    const size_t total = frames + border + chunk;
    const size_t processingEnd = frames + border;
    size_t windowCount = 0;
    for (size_t start = 0; start < processingEnd; start += step)
    {
        ++windowCount;
    }
    if (windowCount == 0)
    {
        return fail("Source contains no processable audio windows");
    }

    KaraokeStorage sums;
    if (!sums.valid())
    {
        return fail(sums.error());
    }
    uint64_t remaining = static_cast<uint64_t>(frames) * 12;
    while (remaining != 0)
    {
        const size_t bytes = static_cast<size_t>((std::min<uint64_t>)(remaining, 1024 * 1024));
        if (sums.append(nullptr, bytes) == UINT64_MAX)
        {
            return fail(sums.error());
        }
        remaining -= bytes;
    }

    std::unique_ptr<void, void (*)(void*)> session(nullptr, &karaokeOrtDestroy);
    char diagnostic[2048] = {0};
    session.reset(karaokeOrtCreate(runtime, model, diagnostic, sizeof(diagnostic)));
    if (session.get() == nullptr)
    {
        return fail(diagnostic);
    }

    int64_t analyzeMs = 0, inferenceMs = 0, reconstructMs = 0;
    size_t completedWindows = 0;
    for (size_t start = 0; start < processingEnd; start += step)
    {
        if (!ReportSeparationProgress(progress, user, 5 + static_cast<int>(static_cast<uint64_t>(completedWindows) * 90 / windowCount)))
        {
            return fail("Vocal separation canceled");
        }
        KaraokeSeparationTask window(&mix, frames, start, total, step, border);
        window.DoTask();
        if (!window.m_error.empty())
        {
            return fail(window.m_error.c_str());
        }
        analyzeMs += window.m_analyzeMs;

        const std::chrono::steady_clock::time_point inferenceStart = std::chrono::steady_clock::now();
        if (!karaokeOrtRun(session.get(), window.m_spectrum.data(), window.m_prediction.data(),
                diagnostic, sizeof(diagnostic)))
        {
            return fail(diagnostic);
        }
        inferenceMs += std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - inferenceStart)
                           .count();

        window.reconstruct();
        window.DoTask();
        if (!window.m_error.empty())
        {
            return fail(window.m_error.c_str());
        }
        reconstructMs += window.m_reconstructMs;
        if (!window.accumulate(&sums))
        {
            return fail(window.m_error.c_str());
        }
        ++completedWindows;
    }

    size_t emitted = 0;
    for (size_t start = 0; start < frames;)
    {
        if (!ReportSeparationProgress(progress, user, 95 + static_cast<int>(emitted * 4 / frames)))
        {
            return fail("Vocal separation canceled");
        }
        const size_t count = (std::min)(chunk, frames - start);
        std::vector<float> values(count * 3), result(count * 2);
        if (!sums.read(start * 12, values.data(), values.size() * sizeof(float)))
        {
            return fail(sums.error());
        }
        for (size_t i = 0; i < count; ++i)
        {
            if (values[i * 3 + 2] <= 0)
            {
                return fail("Uncovered audio during overlap reconstruction");
            }
            for (size_t c = 0; c < 2; ++c)
            {
                result[i * 2 + c] = 1.010f * values[i * 3 + c] / values[i * 3 + 2];
                if (!std::isfinite(result[i * 2 + c]))
                {
                    return fail("Non-finite separated audio");
                }
            }
        }
        if (backing->m_samples->append(result.data(), result.size() * sizeof(float)) == UINT64_MAX)
        {
            return fail(backing->m_samples->error());
        }
        emitted += count;
        start += count;
    }
    if (!ReportSeparationProgress(progress, user, 100))
    {
        return fail("Vocal separation canceled");
    }
    LOGINFO("Separation complete MDX windowFrames=%d overlapFrames=%d windows=%llu totalMs=%lld analyzeMs=%lld inferMs=%lld reconstructMs=%lld",
        windowFrames, overlapFrames, static_cast<unsigned long long>(completedWindows),
        static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count()),
        static_cast<long long>(analyzeMs), static_cast<long long>(inferenceMs), static_cast<long long>(reconstructMs));
    return backing;
}