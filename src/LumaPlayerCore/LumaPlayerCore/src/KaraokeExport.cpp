#include "LumaPlayerCoreC.h"
#include "LumaPlayerCoreCContext.h"
#include "KaraokeExport.h"
#include "KaraokeMediaWriter.h"
#include "KaraokeStorage.h"
#include "KaraokeSeparation.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>

/** Copy an export failure without allocating or unwinding the caller's resources
    @param [in] message Error text, empty uses a fallback
    @param [out] error Optional diagnostic buffer
    @param [in] capacity Buffer size including terminator
    @param [in] result Error code, not success
    @return Supplied error code
    */
int32_t KaraokeExport::exportFailure(const char* message, char* error, size_t capacity,
    int32_t result)
{
    if (error != nullptr && capacity != 0)
    {
        if (message == nullptr || *message == 0)
        {
            message = "Recording export failed without a diagnostic";
        }
        const size_t length = (std::min)(capacity - 1, std::strlen(message));
        std::memcpy(error, message, length);
        error[length] = 0;
    }
    return result;
}

int32_t KaraokeExport::progress(LumaPlayerCoreCSeparationProgress callback, void* user, int percent,
    char* error, size_t capacity)
{
    return callback && !callback(percent, user)
        ? exportFailure("Recording export canceled", error, capacity, LumaPlayerCoreCResultCanceled)
        : LumaPlayerCoreCResultSuccess;
}
int16_t KaraokeExport::saturated(double value)
{
    return static_cast<int16_t>((std::max)(-32768.0, (std::min)(32767.0, value)));
}
int32_t KaraokeExport::decodeBacking(const char* source, LumaPlayerCoreCBacking backing,
    LumaPlayerCoreCSeparationProgress callback, void* user,
    std::shared_ptr<KaraokeStorage>* output, char* error, size_t errorCapacity)
{
    const uint64_t limit = static_cast<uint64_t>(INT64_MAX) / 48000;
    std::shared_ptr<KaraokeStorage> samples(new KaraokeStorage);
    if (!samples->valid())
    {
        return exportFailure(samples->error(), error, errorCapacity);
    }
    if (backing)
    {
        const std::shared_ptr<const KaraokeBacking> value =
            *static_cast<std::shared_ptr<const KaraokeBacking>*>(backing);
        if (!value || value->m_source != source || !value->m_samples)
        {
            return exportFailure("Backing source does not match export source", error, errorCapacity);
        }
        std::vector<float> chunk(44101 * 2);
        uint64_t offset = 0;
        const uint64_t total = value->m_samples->size() / 8;
        while (offset < total)
        {
            if (progress(callback, user, 0, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
            {
                return LumaPlayerCoreCResultCanceled;
            }
            const size_t count = static_cast<size_t>((std::min<uint64_t>)(44101, total - offset));
            if (!value->m_samples->read(offset * 8, chunk.data(), count * 8))
            {
                return exportFailure(value->m_samples->error(), error, errorCapacity);
            }
            const size_t input = (std::min<size_t>)(44100, count);
            const size_t output = (input * 48000 + 44099) / 44100;
            std::vector<int16_t> converted(output * 2);
            for (size_t i = 0; i < output; ++i)
            {
                const double pos = i * (44100.0 / 48000);
                const size_t left = (std::min)(static_cast<size_t>(pos), count - 1);
                const size_t right = (std::min)(left + 1, count - 1);
                for (size_t c = 0; c < 2; ++c)
                {
                    converted[i * 2 + c] = saturated(32768 *
                        (chunk[left * 2 + c] * (1 - (pos - left)) + chunk[right * 2 + c] * (pos - left)));
                }
            }
            if (samples->append(converted.data(), converted.size() * 2) == UINT64_MAX)
            {
                return exportFailure(samples->error(), error, errorCapacity);
            }
            offset += input;
        }
        *output = samples;
        return LumaPlayerCoreCResultSuccess;
    }
    FFmpegCppPlaybackReader reader;
    FFmpegCppPlaybackOpenOption option;
    option.decodeVideo = false;
    option.outputSampleRate = 48000;
    option.outputChannels = 2;
    if (!reader.open(source, option))
    {
        return exportFailure("Cannot open backing audio", error, errorCapacity);
    }
    FFmpegCppPlaybackFrame frame;
    for (;;)
    {
        if (progress(callback, user, 0, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
        {
            return LumaPlayerCoreCResultCanceled;
        }
        FFmpegCppPlaybackReadResult result = reader.read(&frame);
        if (result == FFmpegCppPlaybackReadResultEnd)
        {
            break;
        }
        if (result != FFmpegCppPlaybackReadResultFrame)
        {
            return exportFailure("Backing decode failed", error, errorCapacity);
        }
        if (frame.type != FFmpegCppPlaybackFrameTypeAudio)
        {
            continue;
        }
        const FFmpegCppPlaybackAudioFrame& audio = frame.audioFrame;
        if (audio.sampleRate != 48000 || audio.channels != 2 || audio.pcmData.size() % 4)
        {
            return exportFailure("Unexpected backing PCM format", error, errorCapacity);
        }
        const uint64_t timestamp = static_cast<uint64_t>((std::max<int64_t>)(0, audio.timestamp100ns));
        if (timestamp > limit)
        {
            return exportFailure("Backing timestamp overflow", error, errorCapacity);
        }
        const uint64_t frameOffset = timestamp * 48000 / 10000000;
        const size_t count = audio.pcmData.size() / 2;
        const uint64_t offset = frameOffset * 4;
        while (samples->size() < offset + count * 2)
        {
            if (progress(callback, user, 0, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
            {
                return LumaPlayerCoreCResultCanceled;
            }
            if (samples->append(nullptr, static_cast<size_t>((std::min<uint64_t>)(1024 * 1024, offset + count * 2 - samples->size()))) == UINT64_MAX)
            {
                return exportFailure(samples->error(), error, errorCapacity);
            }
        }
        if (!samples->write(offset, audio.pcmData.data(), audio.pcmData.size()))
        {
            return exportFailure(samples->error(), error, errorCapacity);
        }
    }
    *output = samples;
    return LumaPlayerCoreCResultSuccess;
}

int32_t KaraokeExport::run(LumaPlayerCoreHandle handle, const char* source, const char* destination,
    int32_t includeBacking, int32_t includeVideo, int32_t pitch, int32_t gain, LumaPlayerCoreCBacking backing,
    LumaPlayerCoreCSeparationProgress callback, void* user, char* error, size_t errorCapacity)
{
    try
    {
        if (!handle || !source || !destination || pitch < -7 || pitch > 7 || gain < 0 || gain > 500)
        {
            return exportFailure("Invalid export settings", error, errorCapacity, LumaPlayerCoreCResultInvalidParam);
        }
        if (progress(callback, user, 0, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
        {
            return LumaPlayerCoreCResultCanceled;
        }
        KaraokeRecordingTracks recording;
        {
            LumaPlayerCoreCContext* context = static_cast<LumaPlayerCoreCContext*>(handle);
            std::lock_guard<std::mutex> lock(context->m_karaokeMutex);
            recording = context->m_karaokePcmTimeline;
        }
        FFmpegCppPlaybackReader sourceReader;
        if (!sourceReader.open(source) || !recording.configureSong(sourceReader.duration100ns(), recording.recordingRate()))
        {
            return exportFailure("Cannot map the complete song at the recording rate", error, errorCapacity);
        }
        sourceReader.close();
        uint64_t frames = 0;
        if (recording.sampleRate() != 48000 || recording.channels() != 1 ||
            !recording.outputFrameCount(&frames) || frames == 0 || frames > static_cast<uint64_t>(INT64_MAX) / 10000000)
        {
            return exportFailure("Recording is empty, unsupported or its duration overflows", error, errorCapacity);
        }
        const int64_t duration = frames * 10000000 / 48000;
        const std::vector<KaraokeTimelineSegment> segments = recording.outputSegments();
        if (segments.empty())
        {
            return exportFailure("Recording timeline is invalid", error, errorCapacity);
        }
        KaraokeStorage pcm;
        if (!pcm.valid())
        {
            return exportFailure(pcm.error(), error, errorCapacity);
        }
        std::vector<int16_t> microphone(4096), stereo(4096 * 2);
        for (uint64_t i = 0; i < frames; i += microphone.size())
        {
            if (progress(callback, user, 0, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
            {
                return LumaPlayerCoreCResultCanceled;
            }
            const size_t count = static_cast<size_t>((std::min<uint64_t>)(microphone.size(), frames - i));
            if (!recording.renderOutputFrames(i, count, microphone.data()))
            {
                return exportFailure("Recording snapshot read failed", error, errorCapacity);
            }
            for (size_t n = 0; n < count; ++n)
            {
                stereo[n * 2] = stereo[n * 2 + 1] = microphone[n];
            }
            if (pcm.append(stereo.data(), count * 4) == UINT64_MAX)
            {
                return exportFailure(pcm.error(), error, errorCapacity);
            }
        }
        if (includeBacking)
        {
            std::shared_ptr<KaraokeStorage> background;
            const int32_t decoded = decodeBacking(source, backing, callback, user, &background, error, errorCapacity);
            if (decoded != LumaPlayerCoreCResultSuccess)
            {
                return decoded;
            }
            std::vector<int16_t> input(4096 * 2), output(8192 * 2), mix(8192 * 2);
            for (size_t segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex)
            {
                const KaraokeTimelineSegment& segment = segments[segmentIndex];
                const uint64_t inputStart = segment.m_mediaStart100ns * 48000 / 10000000;
                const uint64_t inputEnd = segment.m_mediaEnd100ns * 48000 / 10000000;
                uint64_t write = segment.m_outputStart100ns * 48000 / 10000000;
                const uint64_t writeEnd = (std::min<uint64_t>)(frames, segment.m_outputEnd100ns * 48000 / 10000000);
                write = (std::min)(write, writeEnd);
                FFmpegCppAudioTempo tempo;
                if (!tempo.init(48000, 2) || !tempo.setRate(segment.m_ratePermille) || !tempo.setPitchSemitones(pitch))
                {
                    return exportFailure("Backing tempo initialization failed", error, errorCapacity);
                }
                for (uint64_t read = inputStart; read < inputEnd || tempo.available();)
                {
                    if (progress(callback, user, 0, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
                    {
                        return LumaPlayerCoreCResultCanceled;
                    }
                    if (read < inputEnd)
                    {
                        const size_t count = static_cast<size_t>((std::min<uint64_t>)(4096, inputEnd - read));
                        std::fill(input.begin(), input.end(), 0);
                        if (read < background->size() / 4)
                        {
                            const size_t available = static_cast<size_t>((std::min<uint64_t>)(count, background->size() / 4 - read));
                            if (!background->read(read * 4, input.data(), available * 4))
                            {
                                return exportFailure(background->error(), error, errorCapacity);
                            }
                        }
                        if (!tempo.write(input.data(), count))
                        {
                            return exportFailure("Backing tempo processing failed", error, errorCapacity);
                        }
                        read += count;
                        if (read == inputEnd && !tempo.finish())
                        {
                            return exportFailure("Backing tempo flush failed", error, errorCapacity);
                        }
                    }
                    while (tempo.available())
                    {
                        const size_t count = tempo.read(output.data(), output.size() / 2);
                        if (!count)
                        {
                            return exportFailure("Backing tempo stalled", error, errorCapacity);
                        }
                        const size_t used = static_cast<size_t>((std::min<uint64_t>)(count, writeEnd - write));
                        if (!pcm.read(write * 4, mix.data(), used * 4))
                        {
                            return exportFailure(pcm.error(), error, errorCapacity);
                        }
                        for (size_t n = 0; n < used * 2; ++n)
                        {
                            mix[n] = saturated(mix[n] + output[n] * (gain / 100.0));
                        }
                        if (!pcm.write(write * 4, mix.data(), used * 4))
                        {
                            return exportFailure(pcm.error(), error, errorCapacity);
                        }
                        write += used;
                    }
                }
            }
        }
        if (progress(callback, user, 10, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
        {
            return LumaPlayerCoreCResultCanceled;
        }
        FFmpegCppPlaybackReader videoReader;
        FFmpegCppPlaybackFrame current, next;
        bool hasNext = false;
        int width = 0, height = 0;
        if (includeVideo)
        {
            FFmpegCppPlaybackOpenOption option;
            option.decodeAudio = false;
            if (!videoReader.open(source, option) || videoReader.read(&current) != FFmpegCppPlaybackReadResultFrame ||
                current.type != FFmpegCppPlaybackFrameTypeVideo)
            {
                return exportFailure("Source video cannot be decoded", error, errorCapacity);
            }
            width = current.videoFrame.width + current.videoFrame.width % 2;
            height = current.videoFrame.height + current.videoFrame.height % 2;
            hasNext = videoReader.read(&next) == FFmpegCppPlaybackReadResultFrame;
        }
        KaraokeMediaWriter writer;
        if (!writer.open(destination, includeVideo != 0, width, height))
        {
            return exportFailure(writer.error().c_str(), error, errorCapacity);
        }
        std::vector<int16_t> encoded(4800 * 2);
        uint64_t audioFrame = 0, videoFrame = 0;
        int64_t videoTime = 0;
        while (audioFrame < frames || (includeVideo && videoTime < duration))
        {
            const int64_t audioTime = audioFrame * 10000000 / 48000;
            if (progress(callback, user, 10 + static_cast<int>((std::min<int64_t>)(audioTime, duration) * 89 / duration), error, errorCapacity) != LumaPlayerCoreCResultSuccess)
            {
                return LumaPlayerCoreCResultCanceled;
            }
            if (audioFrame < frames && (!includeVideo || videoTime >= duration || audioTime <= videoTime))
            {
                const size_t count = static_cast<size_t>((std::min<uint64_t>)(4800, frames - audioFrame));
                if (!pcm.read(audioFrame * 4, encoded.data(), count * 4))
                {
                    return exportFailure(pcm.error(), error, errorCapacity);
                }
                if (!writer.audio(encoded.data(), count, audioTime))
                {
                    return exportFailure(writer.error().c_str(), error, errorCapacity);
                }
                audioFrame += count;
            }
            else
            {
                size_t index = 0;
                while (index + 1 < segments.size() && videoTime >= segments[index].m_outputEnd100ns)
                {
                    ++index;
                }
                const KaraokeTimelineSegment& segment = segments[index];
                const int64_t span = segment.m_outputEnd100ns - segment.m_outputStart100ns;
                if (span <= 0)
                {
                    return exportFailure("Video timeline mapping failed", error, errorCapacity);
                }
                const int64_t mediaTime = segment.m_mediaStart100ns + static_cast<int64_t>(static_cast<long double>(videoTime - segment.m_outputStart100ns) * (segment.m_mediaEnd100ns - segment.m_mediaStart100ns) / span);
                while (hasNext && next.videoFrame.timestamp100ns <= mediaTime)
                {
                    current = std::move(next);
                    const FFmpegCppPlaybackReadResult read = videoReader.read(&next);
                    if (read == FFmpegCppPlaybackReadResultError)
                    {
                        return exportFailure("Video decode failed", error, errorCapacity);
                    }
                    hasNext = read == FFmpegCppPlaybackReadResultFrame;
                }
                std::vector<unsigned char> canvas;
                if (!FFmpegCppHelper::fitBgraFrame(current.videoFrame.bgraPixels, current.videoFrame.width,
                        current.videoFrame.height, current.videoFrame.stride, width, height, &canvas))
                {
                    return exportFailure("Video frame layout cannot be converted", error, errorCapacity);
                }
                const int64_t nextTime = (videoFrame + 1) * 10000000 / 30;
                if (!writer.video(canvas.data(), canvas.size(), videoTime,
                        (std::min)(nextTime, duration) - videoTime))
                {
                    return exportFailure(writer.error().c_str(), error, errorCapacity);
                }
                ++videoFrame;
                videoTime = nextTime;
            }
        }
        if (progress(callback, user, 99, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
        {
            return LumaPlayerCoreCResultCanceled;
        }
        if (!writer.finish())
        {
            return exportFailure(writer.error().c_str(), error, errorCapacity);
        }
        if (progress(callback, user, 100, error, errorCapacity) != LumaPlayerCoreCResultSuccess)
        {
            return LumaPlayerCoreCResultCanceled;
        }
        if (error && errorCapacity)
        {
            error[0] = 0;
        }
        return LumaPlayerCoreCResultSuccess;
    }
    catch (const std::exception& exception)
    {
        return exportFailure(exception.what(), error, errorCapacity);
    }
    catch (...)
    {
        return exportFailure("Unexpected recording export failure", error, errorCapacity);
    }
}

int32_t lumaPlayerCoreKaraokeExport(LumaPlayerCoreHandle handle, const char* source, const char* destination,
    int32_t includeBacking, int32_t includeVideo, int32_t pitch, int32_t gain, LumaPlayerCoreCBacking backing,
    LumaPlayerCoreCSeparationProgress callback, void* user, char* error, size_t errorCapacity)
{
    return KaraokeExport::run(handle, source, destination, includeBacking, includeVideo,
        pitch, gain, backing, callback, user, error, errorCapacity);
}