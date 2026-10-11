#include "KaraokeBackendTests.h"
#include "TestCaseRegistry.h"
#include "TestMediaHelper.h"
#define LUMAPLAYERCORE_USE_C_API
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokeSpectrum.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokePcmTimeline.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokeRecordingTracks.h"
#include "../../LumaPlayerCore/LumaPlayerCore/src/KaraokeMicrophoneControls.h"
#include "FFmpegCpp/FFmpegCppAPI.h"
#include "KissFFTCpp/KissFFTCppAPI.h"
#include <QFile>
#include <QDataStream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>
#include <limits>
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#endif

SeparationTestProgress::SeparationTestProgress(int cancelAt) :
m_last(0),
m_cancelAt(cancelAt),
m_ordered(true)
{

}

int SeparationTestProgress::callback(int percent, void* user)
{
    SeparationTestProgress* state = static_cast<SeparationTestProgress*>(user);
    state->m_ordered = state->m_ordered && percent >= state->m_last && percent <= 100;
    state->m_last = percent;
    return percent < state->m_cancelAt;
}

KaraokeTestCompletion::KaraokeTestCompletion() :
m_done(false),
m_result(-1)
{

}

void KaraokeTestCompletion::receive(const LumaPlayerCoreCCompletion* value, void* user)
{
    KaraokeTestCompletion* state = static_cast<KaraokeTestCompletion*>(user);
    state->m_result = value->m_result;
    state->m_done.store(true);
}

void RunKaraokeBackendCase(int id, TestResults& report, const QString& directory, const QString& fixture)
{
    if (id == CaseKaraokeErrorReturns)
    {
        KaraokeStorage storage;
        report.check(id, storage.valid() && storage.size() == 0 && *storage.error() == 0,
            "temporary storage construction has an explicit valid state");
        if (!storage.valid())
        {
            return;
        }
        const int16_t expected[] = {1234, -2345, 0, 32767};
        int16_t actual[4] = {};
        const size_t bytes = sizeof(expected);
        report.check(id, storage.append(expected, bytes) == 0 && storage.size() == bytes && storage.read(0, actual, bytes) && std::memcmp(expected, actual, bytes) == 0,
            "append and read preserve independently defined PCM samples");
        report.check(id, !storage.read(UINT64_MAX, actual, bytes) && *storage.error() != 0 && !storage.read(1, actual, bytes) && !storage.read(0, nullptr, 1),
            "invalid offsets, crossing EOF and null output return false without throwing");
        report.check(id, !storage.write(UINT64_MAX, expected, bytes) && !storage.write(1, expected, bytes) && !storage.write(0, nullptr, 1) && *storage.error() != 0,
            "invalid scratch writes return false without changing committed size");
        if (static_cast<uint64_t>((std::numeric_limits<size_t>::max)()) > static_cast<uint64_t>(INT64_MAX))
        {
            report.check(id, storage.append(nullptr, (std::numeric_limits<size_t>::max)()) == UINT64_MAX && storage.size() == bytes && *storage.error() != 0,
                "unrepresentable append fails before IO and does not publish a partial range");
        }
        report.check(id, storage.size() == bytes && storage.read(0, actual, bytes) && std::memcmp(expected, actual, bytes) == 0,
            "invalid operations preserve all previously committed samples");
        report.check(id, storage.read(bytes, nullptr, 0) && storage.write(bytes, nullptr, 0) && storage.append(nullptr, 0) == bytes && storage.size() == bytes,
            "empty EOF operations succeed without dereferencing null data");
        report.check(id, storage.append(nullptr, bytes) == bytes && storage.read(bytes, actual, bytes) && std::all_of(actual, actual + 4, [](int16_t sample)
                                                                                                              {
                                                                                                                  return sample == 0;
                                                                                                              }),
            "valid silence append still works after rejected operations");
        report.check(id, storage.write(bytes, expected, bytes) && storage.read(bytes, actual, bytes) && std::memcmp(expected, actual, bytes) == 0,
            "valid scratch write and read recover after invalid input");

        char error[128] = {};
        LumaPlayerCoreCSeparationOptions options = {0, 261120, 130560};
        const LumaPlayerCoreCBacking invalid = lumaPlayerCoreSeparateVocalsEx(nullptr, nullptr, nullptr,
            32 * 1024 * 1024, &options, nullptr, nullptr, error, sizeof(error));
        report.check(id, !invalid && QByteArray(error).contains("Invalid separation options"),
            "invalid separation options return a diagnostic before touching model files");
        lumaPlayerCoreBackingDestroy(invalid);
        char bounded[] = {'L', 'x', 'R'};
        const LumaPlayerCoreCBacking tinyBacking = lumaPlayerCoreSeparateVocalsEx(nullptr, nullptr, nullptr,
            32 * 1024 * 1024, &options, nullptr, nullptr, bounded + 1, 1);
        report.check(id, !tinyBacking && bounded[0] == 'L' && bounded[1] == 0 && bounded[2] == 'R',
            "one-byte diagnostic is terminated without overwriting neighboring canaries");
        lumaPlayerCoreBackingDestroy(tinyBacking);
        options.m_size = sizeof(options);
        const LumaPlayerCoreCBacking missing = lumaPlayerCoreSeparateVocalsEx(nullptr, nullptr, nullptr,
            32 * 1024 * 1024, &options, nullptr, nullptr, error, sizeof(error));
        report.check(id, !missing && QByteArray(error).contains("Invalid MDX settings"),
            "invalid source/runtime/model pointers produce a normal failure");
        lumaPlayerCoreBackingDestroy(missing);
        report.check(id, lumaPlayerCoreKaraokeExport(nullptr, nullptr, nullptr, 0, 0, 0, 100, nullptr, nullptr, nullptr, error, sizeof(error)) == LumaPlayerCoreCResultInvalidParam && *error != 0,
            "invalid export settings return InvalidParam with a diagnostic");
        LumaPlayerCoreHandle core = lumaPlayerCoreCreate();
        report.check(id, core != nullptr, "independent Core created for pre-IO cancellation");
        if (core != nullptr)
        {
            SeparationTestProgress cancel(0);
            const QString path = directory + "/canceled-export.mp3";
            report.check(id, lumaPlayerCoreKaraokeExport(core, "unused-source", path.toUtf8().constData(), 0, 0, 0, 100, nullptr, SeparationTestProgress::callback, &cancel, error, sizeof(error)) == LumaPlayerCoreCResultCanceled && QByteArray(error).contains("canceled") && !QFile::exists(path),
                "cancel returns Canceled and creates no output file before touching source audio");
            lumaPlayerCoreDestroy(core);
        }
        return;
    }
    if (id == CaseKaraokePortableWriter)
    {
        FFmpegCppMediaWriter invalid;
        report.check(id, !invalid.open("", false, 0, 0) && !invalid.finish() && !invalid.error().empty(),
            "portable writer rejects empty path and finishing unopened output with explicit error");
        for (int video = 0; video <= 1; ++video)
        {
            FFmpegCppMediaWriter writer;
            const QString path = directory + (video ? "/portable.mp4" : "/portable.mp3");
            const bool opened = writer.open(path.toUtf8().constData(), video != 0, 160, 90);
            if (!opened)
            {
                report.check(id, writer.error().find("encoder") != std::string::npos,
                    "host FFmpeg has no usable codec; explicit diagnostic (not encoded-output coverage): " + QString::fromStdString(writer.error()));
                continue;
            }
            std::vector<int16_t> audio(3200, 2000);
            std::vector<unsigned char> pixels(160 * 90 * 4, 127);
            bool valid = true;
            for (int i = 0; i < 30 && valid; ++i)
            {
                valid = writer.audio(audio.data(), 1600) && (!video || writer.video(pixels.data(), pixels.size(), i * 10000000LL / 30));
            }
            valid = valid && writer.finish();
            report.check(id, valid, "portable encoded output completed: " + QString::fromStdString(writer.error()));
            FFmpegCppPlaybackReader reader;
            FFmpegCppPlaybackFrame frame;
            bool decoded = valid && reader.open(path.toUtf8().constData());
            int64_t end = 0;
            if (decoded)
            {
                for (;;)
                {
                    const FFmpegCppPlaybackReadResult result = reader.read(&frame);
                    if (result == FFmpegCppPlaybackReadResultEnd)
                    {
                        break;
                    }
                    if (result != FFmpegCppPlaybackReadResultFrame)
                    {
                        decoded = false;
                        break;
                    }
                    if (frame.type == FFmpegCppPlaybackFrameTypeAudio)
                    {
                        end = frame.audioFrame.timestamp100ns + frame.audioFrame.duration100ns;
                    }
                }
            }
            report.check(id, decoded && std::abs(end - 10000000) < 1000000,
                QString("portable encoded stream actually decodes with expected duration: %1").arg(end));
        }
        return;
    }
    if (id == CaseKaraokeDiskRecording)
    {
        KaraokePcmTimeline recording;
        bool valid = recording.init(48000, 1, 0);
        std::vector<int16_t> block(48000, 1234);
        for (int64_t second = 0; valid && second < 3000; ++second)
        {
            valid = recording.replaceCapturedRange(second * 10000000, (second + 1) * 10000000, 1000, block.data(), block.size());
        }
        uint64_t frames = 0;
        report.check(id, valid && recording.outputFrameCount(&frames) && frames == 144000000 && recording.storedPcmBytes() == 0,
            "50-minute generated recording exceeds old 256MiB cap without retaining PCM in RAM");
        KaraokePcmTimeline snapshot = recording;
        std::fill(block.begin(), block.end(), 4321);
        report.check(id, recording.replaceCapturedRange(10000000, 20000000, 1000, block.data(), block.size()), "disk span supports overwrite");
        int16_t original = 0, replaced = 0, tail = 0;
        report.check(id, snapshot.renderOutputFrames(48000, 1, &original) && original == 1234 && recording.renderOutputFrames(48000, 1, &replaced) && replaced == 4321 && recording.renderOutputFrames(143999999, 1, &tail) && tail == 1234,
            "snapshot keeps original samples while overwrite and long-file tail remain readable");
        recording.clear();
        report.check(id, snapshot.renderOutputFrames(143999999, 1, &tail) && tail == 1234, "clearing live recording does not destroy export snapshot");
        KaraokePcmTimeline drift;
        drift.init(48000, 1, 0);
        drift.replaceCapturedRange(10000000, 30000000, 2000, block.data(), 47990);
        const std::vector<KaraokeTimelineSegment> segments = drift.outputSegments();
        report.check(id, segments.size() == 2 && segments.back().m_outputEnd100ns * 48000 / 10000000 == 95990,
            "video mapping uses actual PCM count, not nominal media duration rounding");
        return;
    }
    if (id == CaseKaraokeRecordingClock)
    {
        LumaPlayerCoreHandle core = lumaPlayerCoreCreate();
        bool valid = core && lumaPlayerCoreInit(core) == 0 && lumaPlayerCoreOpenMedia(core, fixture.toUtf8().constData()) == 0;
        LumaPlayerCoreCSnapshot previous = {}, current = {};
        lumaPlayerCoreGetSnapshot(core, &previous);
        for (int action = 0; valid && action < 4; ++action)
        {
            const int result = action == 0 ? lumaPlayerCorePlay(core) : action == 1 ? lumaPlayerCorePause(core)
                : action == 2                                                       ? lumaPlayerCoreSetPlaybackRatePermille(core, 2000)
                                                                                    : lumaPlayerCoreSeekTo(core, 500000, 0);
            lumaPlayerCoreGetSnapshot(core, &current);
            report.check(id, result == 0 && current.m_recordingSerial > previous.m_recordingSerial && current.m_recordingPosition100ns >= 0,
                QString("recording discontinuity serial changes on play/pause/rate/tiny seek: %1").arg(action));
            previous = current;
        }
        report.check(id, valid, "real Core opened generated fixture for recording clock checks");
        lumaPlayerCoreDestroy(core);
        return;
    }
    if (id == CaseKaraokeRecordingEof)
    {
        const QString source = directory + "/recording-eof.avi";
        QFile file(source);
        std::atomic<bool> cancel(false);
        const QByteArray media = TestMediaHelper::generate(cancel);
        const bool created = file.open(QIODevice::WriteOnly) && file.write(media) == media.size();
        file.close();
        LumaPlayerCoreHandle core = lumaPlayerCoreCreate();
        bool ready = created && core && lumaPlayerCoreInit(core) == 0 &&
            lumaPlayerCoreOpenMedia(core, source.toUtf8().constData()) == 0 &&
            lumaPlayerCoreSetLoopAAtPosition(core, 10000000) == 0 &&
            lumaPlayerCoreSetLoopBAtPosition(core, 20000000) == 0 &&
            lumaPlayerCoreSetPlaybackRatePermille(core, 5000) == 0;
        report.check(id, ready, "generated video opened with real AB range and 5x playback");
        if (!ready)
        {
            lumaPlayerCoreDestroy(core);
            return;
        }
        KaraokeTestCompletion completion;
        for (int mode = 1; mode >= 0; --mode)
        {
            LumaPlayerCoreCRequest request = {};
            request.m_requestId = 5900 + mode;
            request.m_operation = LumaPlayerCoreCOperationKaraokeRecording;
            request.m_ratePermille = mode;
            completion.m_done.store(false);
            const bool submitted = lumaPlayerCoreSubmitAsyncEx(core, &request, KaraokeTestCompletion::receive, &completion) == 0;
            for (int i = 0; submitted && !completion.m_done.load() && i < 400; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            const bool applied = submitted && completion.m_done.load() && completion.m_result == 0;
            report.check(id, applied, QString("recording mode %1 applied through command queue").arg(mode));
            if (!applied)
            {
                lumaPlayerCoreDestroy(core);
                return;
            }
            const int rateResult = lumaPlayerCoreSetPlaybackRatePermille(core, 2000);
            LumaPlayerCoreCSnapshot lockedSnapshot = {};
            lumaPlayerCoreGetSnapshot(core, &lockedSnapshot);
            report.check(id, mode ? rateResult != 0 && lockedSnapshot.m_ratePermille == 5000 : rateResult == 0,
                "recording locks rate, including paused state; stopping unlocks it");
            if (!mode)
            {
                lumaPlayerCoreSetPlaybackRatePermille(core, 5000);
            }
            lumaPlayerCoreSeekTo(core, 0, 0);
            lumaPlayerCorePlay(core);
            LumaPlayerCoreCSnapshot snapshot = {};
            for (int i = 0; i < 230; ++i)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                lumaPlayerCoreGetSnapshot(core, &snapshot);
                if (mode && snapshot.m_state == LumaPlayerCoreCStateEnded)
                {
                    break;
                }
            }
            report.check(id, snapshot.m_hasLoopA && snapshot.m_hasLoopB, "recording mode does not delete original AB markers");
            report.check(id, mode ? snapshot.m_state == LumaPlayerCoreCStateEnded && snapshot.m_position100ns >= 79000000 : snapshot.m_state == LumaPlayerCoreCStatePlaying && snapshot.m_position100ns < 25000000,
                QString("recording reaches EOF without loop; normal mode restores AB; mode=%1 state=%2 position=%3")
                    .arg(mode)
                    .arg(snapshot.m_state)
                    .arg(snapshot.m_position100ns));
            lumaPlayerCorePause(core);
        }
        lumaPlayerCoreDestroy(core);
        return;
    }
    if (id == CaseKaraokeExport || id == CaseKaraokeOddVideo)
    {
        const QString generated = directory + "/export-source.avi";
        QFile sourceFile(generated);
        std::atomic<bool> cancel(false);
        const QByteArray media = TestMediaHelper::generate(cancel, true, id == CaseKaraokeOddVideo);
        bool created = sourceFile.open(QIODevice::WriteOnly) && sourceFile.write(media) == media.size();
        sourceFile.close();
        report.check(id, created, "test program generates its own source video/audio");
        if (!created)
        {
            return;
        }
        LumaPlayerCoreHandle core = lumaPlayerCoreCreate();
        std::vector<int16_t> recording(48000);
        for (size_t i = 0; i < recording.size(); ++i)
        {
            recording[i] = static_cast<int16_t>(7000 * std::sin(6.283185307179586 * 440 * i / 48000));
        }
        bool ready = core && lumaPlayerCoreKaraokeInit(core, 48000, 1, 1024 * 1024) == 0 &&
            lumaPlayerCoreKaraokeWritePcm(core, 10000000, 30000000, 2000, recording.data(), recording.size()) == 0;
        report.check(id, ready, "1s capture at source 1..3s, 2x; whole 8s source must export 4s");
        if (!ready)
        {
            lumaPlayerCoreDestroy(core);
            return;
        }
        std::vector<int16_t> extra(recording.size()), monitor(recording.size()), expected(recording), actual(recording.size());
        for (int slot = 1; slot < 3; ++slot)
        {
            for (size_t i = 0; i < extra.size(); ++i)
            {
                extra[i] = static_cast<int16_t>((slot == 1 ? -2000 : 1000) * std::sin(6.283185307179586 * 440 * i / 48000));
                expected[i] += extra[i];
            }
            report.check(id, lumaPlayerCoreKaraokeProcessMicrophoneSlot(core, slot, 10000000, 30000000, 2000, extra.data(), extra.size(), 100, 0, 0, 1, 1, monitor.data()) == 0,
                QString("microphone lane %1 writes through public Core API").arg(slot + 1));
        }
        // Before export fixes the complete song, the legacy API maps skipped prefix at 1x.
        report.check(id, lumaPlayerCoreKaraokeReadPcm(core, 48000, actual.size(), actual.data()) == 0 && actual == expected,
            "export source contains sample-exact sum of all three microphones, not only the selected lane");
        report.check(id, lumaPlayerCoreKaraokeProcessMicrophoneSlot(core, 3, 10000000, 30000000, 2000, extra.data(), extra.size(), 100, 0, 0, 1, 1, monitor.data()) != 0, "public Core API rejects fourth lane");
        for (int mode = id == CaseKaraokeOddVideo ? 2 : 0; mode < 4; ++mode)
        {
            const bool video = mode >= 2, mix = mode % 2 != 0;
            const QString path = directory + QString("/export-%1.").arg(mode) + (video ? "mp4" : "mp3");
            char error[2048] = {};
            SeparationTestProgress progress;
            const int result = lumaPlayerCoreKaraokeExport(core, generated.toUtf8().constData(), path.toUtf8().constData(),
                mix, video, 0, 100, nullptr, SeparationTestProgress::callback, &progress, error, sizeof(error));
            report.check(id, result == 0, QString("mode %1 encode succeeded: %2").arg(mode).arg(QString::fromUtf8(error)));
            if (result != 0)
            {
                continue;
            }
            FFmpegCppPlaybackReader reader;
            bool decoded = reader.open(path.toUtf8().constData());
            FFmpegCppPlaybackFrame frame;
            int64_t audioEnd = 0, videoEnd = 0;
            double energy = 0, prefixEnergy = 0, tailEnergy = 0;
            bool earlyVideo = false, lateVideo = false;
            bool dimensions = true;
            while (decoded)
            {
                const FFmpegCppPlaybackReadResult read = reader.read(&frame);
                if (read == FFmpegCppPlaybackReadResultEnd)
                {
                    break;
                }
                if (read == FFmpegCppPlaybackReadResultError)
                {
                    decoded = false;
                    break;
                }
                if (frame.type == FFmpegCppPlaybackFrameTypeAudio)
                {
                    audioEnd = frame.audioFrame.timestamp100ns + frame.audioFrame.duration100ns;
                    for (size_t i = 0; i + 1 < frame.audioFrame.pcmData.size(); i += 2)
                    {
                        int16_t sample;
                        std::memcpy(&sample, &frame.audioFrame.pcmData[i], 2);
                        energy += static_cast<double>(sample) * sample;
                        if (frame.audioFrame.timestamp100ns > 1000000 && frame.audioFrame.timestamp100ns < 3000000)
                        {
                            prefixEnergy += static_cast<double>(sample) * sample;
                        }
                        if (frame.audioFrame.timestamp100ns > 30000000 && frame.audioFrame.timestamp100ns < 35000000)
                        {
                            tailEnergy += static_cast<double>(sample) * sample;
                        }
                    }
                }
                if (frame.type == FFmpegCppPlaybackFrameTypeVideo)
                {
                    const FFmpegCppPlaybackVideoFrame& image = frame.videoFrame;
                    dimensions = dimensions && image.width == (id == CaseKaraokeOddVideo ? 162 : 160) &&
                        image.height == (id == CaseKaraokeOddVideo ? 92 : 90);
                    videoEnd = image.timestamp100ns + image.duration100ns;
                    const size_t top = (image.height / 4) * image.stride + (image.width / 2) * 4;
                    const size_t bottom = (image.height * 3 / 4) * image.stride + (image.width / 2) * 4;
                    if (bottom + 2 < image.bgraPixels.size())
                    {
                        if (image.timestamp100ns > 1000000 && image.timestamp100ns < 3000000)
                        {
                            earlyVideo = image.bgraPixels[top + 2] > 180 && image.bgraPixels[bottom] > 180;
                        }
                        if (image.timestamp100ns > 34000000 && image.timestamp100ns < 36000000)
                        {
                            lateVideo = image.bgraPixels[top] > 180 && image.bgraPixels[bottom + 2] > 180;
                        }
                    }
                }
            }
            report.check(id, decoded && energy > 0 && std::abs(audioEnd - 40000000) < 1000000,
                QString("mode %1 actual decoded PCM nonzero, duration within codec padding: %2").arg(mode).arg(audioEnd));
            report.check(id, video ? std::abs(videoEnd - 40000000) < 400000 : videoEnd == 0,
                QString("mode %1 actual video stream/duration: %2").arg(mode).arg(videoEnd));
            report.check(id, mix ? prefixEnergy > 1000000 : prefixEnergy < 1000,
                QString("mode %1 unrecorded prefix is silent for microphone-only but retains backing for mix; energy=%2").arg(mode).arg(prefixEnergy));
            report.check(id, mix ? tailEnergy > 1000000 : tailEnergy < 1000,
                QString("mode %1 unrecorded tail: microphone silent, backing retained; energy=%2").arg(mode).arg(tailEnergy));
            if (video)
            {
                report.check(id, earlyVideo && lateVideo && dimensions, "actual output frames preserve dimensions, vertical orientation and original-time color changes");
            }
            report.check(id, progress.m_ordered && progress.m_last == 100, "export progress follows actual work");
        }
        lumaPlayerCoreDestroy(core);
        return;
    }
    if (id == CaseKaraokeNativeBoundary)
    {
        KaraokeRecordingTracks tracks;
        std::vector<int16_t> lane(480, 20000), mixed(480);
        report.check(id, tracks.init(48000, 1, 0) && tracks.configureSong(10000000, 1000), "three recording lanes initialize with complete one second song");
        bool writes = tracks.replaceCapturedRange(0, 100000, 1000, lane.data(), lane.size(), 0) &&
            tracks.replaceCapturedRange(0, 100000, 1000, lane.data(), lane.size(), 1);
        std::fill(lane.begin(), lane.end(), -10000);
        writes = writes && tracks.replaceCapturedRange(0, 100000, 1000, lane.data(), lane.size(), 2);
        report.check(id, writes && tracks.renderOutputFrames(0, mixed.size(), mixed.data()) && std::count(mixed.begin(), mixed.end(), 30000) == mixed.size(), "simultaneous lanes sum once without intermediate clipping or overwriting each other");
        std::fill(lane.begin(), lane.end(), 1000);
        report.check(id, tracks.replaceCapturedRange(0, 100000, 1000, lane.data(), lane.size(), 1) && tracks.renderOutputFrames(0, mixed.size(), mixed.data()) && std::count(mixed.begin(), mixed.end(), 11000) == mixed.size(),
            "overwriting one microphone preserves other microphone samples");
        report.check(id, !tracks.replaceCapturedRange(0, 100000, 1000, lane.data(), lane.size(), -1) && !tracks.replaceCapturedRange(0, 100000, 1000, lane.data(), lane.size(), 3), "invalid microphone lane rejected");
        uint64_t total = 0;
        report.check(id, tracks.outputFrameCount(&total) && total == 48000 && tracks.renderOutputFrames(47520, mixed.size(), mixed.data()) && std::count(mixed.begin(), mixed.end(), 0) == mixed.size(),
            "all unrecorded song tail remains silent and full length is retained");
        const uint32_t controls1020 = KaraokeMicrophoneControls::encode(1020, false, true);
        const uint32_t controls1025 = KaraokeMicrophoneControls::encode(1025, false, true);
        const uint32_t controls2000 = KaraokeMicrophoneControls::encode(2000, false, true);
        const bool gainBitsValid = KaraokeMicrophoneControls::gain(controls1020) == 1020 &&
            KaraokeMicrophoneControls::gain(controls1025) == 1025 &&
            KaraokeMicrophoneControls::gain(controls2000) == 2000 &&
            !KaraokeMicrophoneControls::muted(controls1025) &&
            !KaraokeMicrophoneControls::muted(controls2000) &&
            KaraokeMicrophoneControls::monitoring(controls2000);
        report.check(id, gainBitsValid, "gain values above former mute-bit boundary remain audible and retain exact values");
        bool independent = true;
        const int gains[] = {0, 100, 1000, 1023, 1024, 2000, 2047, 2048, 4095, 4096, 5000};
        for (size_t i = 0; i < sizeof(gains) / sizeof(gains[0]); ++i)
        {
            for (int flags = 0; flags < 4; ++flags)
            {
                const uint32_t controls = KaraokeMicrophoneControls::encode(gains[i], (flags & 1) != 0, (flags & 2) != 0);
                independent = independent && KaraokeMicrophoneControls::gain(controls) == gains[i] &&
                    KaraokeMicrophoneControls::muted(controls) == ((flags & 1) != 0) &&
                    KaraokeMicrophoneControls::monitoring(controls) == ((flags & 2) != 0);
            }
        }
        report.check(id, independent, "0..5000 gain boundaries cannot alias mute or monitor flags");
        LumaPlayerCoreCMicrophone mic = lumaPlayerCoreMicrophoneCreate();
        report.check(id, mic != nullptr, "native handle allocated without starting microphone");
        if (mic == nullptr)
        {
            return;
        }
        LumaPlayerCoreCMicrophoneStats stats = {};
        report.check(id, lumaPlayerCoreMicrophoneStats(mic, &stats) == 0 && !stats.m_running,
            "native microphone initially stopped");
        report.check(id, lumaPlayerCoreMicrophoneControls(mic, -1, 0, 1) != 0 && lumaPlayerCoreMicrophoneControls(mic, 5001, 0, 1) != 0, "reject out-of-range gain");
        report.check(id, lumaPlayerCoreMicrophoneControls(mic, 0, 1, 0) == 0 && lumaPlayerCoreMicrophoneControls(mic, 5000, 0, 1) == 0, "gain endpoints and independent controls");
        report.check(id, lumaPlayerCoreMicrophoneStats(nullptr, &stats) != 0 && lumaPlayerCoreMicrophoneStats(mic, nullptr) != 0, "null stats/handle rejected");
        int16_t output[64] = {};
        report.check(id, lumaPlayerCoreMicrophoneRead(mic, output, 64, nullptr) == 0,
            "stopped microphone has no fabricated audio");
        lumaPlayerCoreMicrophoneStop(mic);
        lumaPlayerCoreMicrophoneStop(mic);
        lumaPlayerCoreMicrophoneDestroy(mic);
        lumaPlayerCoreMicrophoneDestroy(nullptr);
        report.check(id, true, "repeated stop and null destruction safe");
        return;
    }
    if (id == CaseKaraokeSpectrum)
    {
        std::vector<double> impulse(16, 0), restored(16, 0);
        std::vector<KissFFTCppComplex> frequencies(9);
        impulse[0] = 1;
        KissFFTCpp forward, backward;
        report.check(id, !forward.forward(impulse.data(), impulse.size(), frequencies.data(), frequencies.size()),
            "FFT wrapper rejects an uninitialized plan");
        const bool planned = forward.open(16, false) && backward.open(16, true);
        report.check(id, planned, "FFT wrapper creates independent forward/inverse plans");
        if (!planned)
        {
            return;
        }
        report.check(id, !forward.open(0, false) && !forward.open(-2, false)
            && !forward.open(3, false) && !forward.open(268435456, false)
            && !forward.open((std::numeric_limits<int32_t>::max)(), false),
            "FFT wrapper rejects invalid and overflow lengths without replacing the valid plan");
        report.check(id, !forward.forward(nullptr, 16, frequencies.data(), 9)
            && !forward.forward(impulse.data(), 15, frequencies.data(), 9)
            && !forward.forward(impulse.data(), 16, nullptr, 9)
            && !forward.forward(impulse.data(), 16, frequencies.data(), 8)
            && frequencies[0].real == 0 && frequencies[0].imaginary == 0,
            "FFT wrapper rejects invalid forward buffers without changing output");
        bool transformed = forward.forward(impulse.data(), impulse.size(), frequencies.data(), frequencies.size());
        for (size_t i = 0; i < frequencies.size(); ++i)
        {
            transformed = transformed && std::abs(frequencies[i].real - 1) < 1e-12
                && std::abs(frequencies[i].imaginary) < 1e-12;
        }
        report.check(id, transformed, "independent impulse oracle: every FFT bin is one, with zero imaginary part");
        report.check(id, !forward.inverse(frequencies.data(), 9, restored.data(), 16)
            && !backward.forward(impulse.data(), 16, frequencies.data(), 9)
            && !backward.inverse(nullptr, 9, restored.data(), 16)
            && !backward.inverse(frequencies.data(), 8, restored.data(), 16)
            && !backward.inverse(frequencies.data(), 9, nullptr, 16)
            && !backward.inverse(frequencies.data(), 9, restored.data(), 15)
            && restored[0] == 0,
            "FFT wrapper rejects direction and inverse buffer errors without changing output");
        bool recovered = backward.inverse(frequencies.data(), frequencies.size(), restored.data(), restored.size());
        for (size_t i = 0; i < restored.size(); ++i)
        {
            recovered = recovered && std::abs(restored[i] / 16 - impulse[i]) < 1e-12;
        }
        report.check(id, recovered, "FFT inverse preserves native normalization and reconstructs the impulse");
        report.check(id, forward.open(32, false) && forward.open(16, false)
            && forward.forward(impulse.data(), impulse.size(), frequencies.data(), frequencies.size()),
            "FFT wrapper replaces and releases plans and remains usable");
        const size_t frames = 261120;
        std::vector<float> audio(frames * 2), spectrum, silence(4 * 2560 * 256, 0), output;
        for (size_t i = 0; i < frames; ++i)
        {
            audio[i * 2] = static_cast<float>(0.4 * std::sin(6.283185307179586 * i * 32 / 5120));
            audio[i * 2 + 1] = static_cast<float>(0.2 * std::cos(6.283185307179586 * i * 79 / 5120));
        }
        const bool analyzed = KaraokeSpectrum::analyze(audio, &spectrum);
        report.check(id, analyzed && spectrum.size() == silence.size(), "MDX tensor shape [1,4,2560,256]");
        if (!analyzed)
        {
            return;
        }
        const size_t bin = 32 * 256 + 10;
        const double magnitude = std::hypot(spectrum[bin], spectrum[bin + 2560 * 256]);
        report.check(id, std::abs(magnitude - 512.0) < 0.001,
            QString("independent Hann sinusoid magnitude=512 actual=%1").arg(magnitude, 0, 'g', 12));
        const size_t rightBin = (2 * 2560 + 79) * 256 + 10;
        report.check(id, std::abs(std::hypot(spectrum[rightBin], spectrum[rightBin + 2560 * 256]) - 256.0) < 0.001,
            "right channel real/imaginary planes independently match magnitude=256");
        // Centered frame 10 starts at 7680: bin 32 has 48 cycles, bin 79 has 118.5 cycles.
        report.check(id, std::abs(spectrum[bin]) < 0.001 && std::abs(spectrum[bin + 2560 * 256] + 512.0) < 0.001 && std::abs(spectrum[rightBin] + 256.0) < 0.001 && std::abs(spectrum[rightBin + 2560 * 256]) < 0.001,
            "independent centered-window phases verify plane order and imaginary sign");
        const bool synthesized = KaraokeSpectrum::synthesize(spectrum, &output);
        double maximum = 0;
        if (synthesized)
        {
            for (size_t i = 10240; i < audio.size() - 10240; ++i)
            {
                maximum = (std::max)(maximum, static_cast<double>(std::abs(output[i] - audio[i])));
            }
        }
        report.check(id, synthesized && output.size() == audio.size() && maximum < 0.000002,
            QString("direct spectrum preserves both channels/length; max interior error=%1").arg(maximum, 0, 'g', 12));
        bool zero = KaraokeSpectrum::synthesize(silence, &output);
        for (size_t i = 0; i < output.size(); ++i)
        {
            zero = zero && output[i] == 0;
        }
        report.check(id, zero, "zero direct spectrum produces silence including both boundaries");
        audio.pop_back();
        report.check(id, !KaraokeSpectrum::analyze(audio, &spectrum) && !KaraokeSpectrum::synthesize(std::vector<float>(), &output), "invalid sizes rejected");
        return;
    }
    const QString source = directory + "/separation-generated.wav";
    const int frames = 44100 * 25 + 12345;
    const bool written = TestMediaHelper::writeSeparationWave(source, frames);
    report.check(id, written, "test program generates stereo nonzero source; no user recording used");
    if (!written)
    {
        return;
    }
#ifdef _WIN32
    wchar_t selectedRuntime[32768] = {};
    wchar_t systemDirectory[32768] = {};
    const HMODULE runtimeModule = GetModuleHandleW(L"msvcp140.dll");
    const bool systemRuntime = runtimeModule != nullptr &&
        GetModuleFileNameW(runtimeModule, selectedRuntime, 32768) != 0 &&
        GetSystemDirectoryW(systemDirectory, 32768) != 0 &&
        QString::fromWCharArray(selectedRuntime).compare(QString::fromWCharArray(systemDirectory) + "\\msvcp140.dll", Qt::CaseInsensitive) == 0;
    report.check(id, systemRuntime, "ONNX dependencies select system CRT even after optional shell-extension fixture: " + QString::fromWCharArray(selectedRuntime));
#endif
    const QByteArray runtime = qgetenv("LUMAPLAYER_ORT_RUNTIME"), model = qgetenv("LUMAPLAYER_VOCAL_MODEL");
    char error[2048] = {};
    const QString invalidModel = directory + "/invalid-model.onnx";
    QFile corrupt(invalidModel);
    const bool corruptWritten = corrupt.open(QIODevice::WriteOnly) && corrupt.write("not an ONNX model") == 17;
    corrupt.close();
    LumaPlayerCoreCBacking invalid = corruptWritten ? lumaPlayerCoreSeparateVocals(source.toUtf8().constData(),
                                                          runtime.constData(), invalidModel.toUtf8().constData(), 32 * 1024 * 1024, nullptr, nullptr, error, sizeof(error))
                                                    : nullptr;
    report.check(id, corruptWritten && !invalid && error[0] != 0, "corrupt model yields a diagnostic and no installable backing");
    lumaPlayerCoreBackingDestroy(invalid);
    invalid = lumaPlayerCoreSeparateVocals(source.toUtf8().constData(),
        runtime.constData(), (directory + "/missing-model.onnx").toUtf8().constData(),
        32 * 1024 * 1024, nullptr, nullptr, error, sizeof(error));
    report.check(id, !invalid && error[0] != 0, "missing model yields a diagnostic and no backing");
    lumaPlayerCoreBackingDestroy(invalid);
    SeparationTestProgress canceled(0);
    LumaPlayerCoreCBacking result = lumaPlayerCoreSeparateVocals(source.toUtf8().constData(),
        runtime.constData(), model.constData(), 32 * 1024 * 1024, SeparationTestProgress::callback, &canceled, error, sizeof(error));
    report.check(id, result == nullptr && QByteArray(error).contains("canceled"), "cancel before model initialization returns no result");
    lumaPlayerCoreBackingDestroy(result);
    SeparationTestProgress progress;
    LumaPlayerCoreCSeparationOptions baselineOptions = {sizeof(baselineOptions), 261120, 130560};
    result = lumaPlayerCoreSeparateVocalsEx(source.toUtf8().constData(), runtime.constData(), model.constData(),
        32 * 1024 * 1024, &baselineOptions, SeparationTestProgress::callback, &progress, error, sizeof(error));
    report.check(id, result != nullptr, QString("nonzero audio runs complete model pipeline: %1").arg(QString::fromUtf8(error)));
    if (result)
    {
        std::vector<float> output(frames * 2);
        size_t count = lumaPlayerCoreBackingRead(result, 0, output.data(), frames + 0);
        bool finite = count == frames;
        double energy = 0;
        for (size_t i = 0; i < count * 2; ++i)
        {
            finite = finite && std::isfinite(output[i]);
            energy += output[i] * output[i];
        }
        report.check(id, finite && energy > 0, "backing has finite nonzero PCM and original duration");
        report.check(id, progress.m_ordered && progress.m_last == 100, "progress ordered and reaches 100 only after final reconstruction");
        report.check(id, lumaPlayerCoreBackingRead(result, frames, output.data(), 1) == 0,
            "read at EOF bounded");
        LumaPlayerCoreCSeparationOptions cancelOptions = {sizeof(cancelOptions), 261120, 130560};
        SeparationTestProgress middleCancel(25);
        LumaPlayerCoreCBacking canceledResult = lumaPlayerCoreSeparateVocalsEx(source.toUtf8().constData(),
            runtime.constData(), model.constData(), 32 * 1024 * 1024, &cancelOptions,
            SeparationTestProgress::callback, &middleCancel, error, sizeof(error));
        report.check(id, !canceledResult && middleCancel.m_last >= 25 && middleCancel.m_last < 100 && QByteArray(error).contains("canceled"), "cancel between serial inference windows returns no partial backing");
        lumaPlayerCoreBackingDestroy(canceledResult);
        cancelOptions.m_overlapFrames = 261120;
        canceledResult = lumaPlayerCoreSeparateVocalsEx(source.toUtf8().constData(), runtime.constData(), model.constData(),
            32 * 1024 * 1024, &cancelOptions, nullptr, nullptr, error, sizeof(error));
        report.check(id, !canceledResult && QByteArray(error).contains("Invalid"), "overlap not shorter than the model window is rejected");
        lumaPlayerCoreBackingDestroy(canceledResult);
        const QString shortSource = directory + "/separation-short.wav";
        const int shortFrames = 44100 * 3 + 17;
        const bool shortWritten = TestMediaHelper::writeSeparationWave(shortSource, shortFrames);
        SeparationTestProgress shortProgress;
        LumaPlayerCoreCBacking shortResult = shortWritten ? lumaPlayerCoreSeparateVocalsEx(shortSource.toUtf8().constData(),
                                                                runtime.constData(), model.constData(), 32 * 1024 * 1024, &baselineOptions,
                                                                SeparationTestProgress::callback, &shortProgress, error, sizeof(error))
                                                          : nullptr;
        std::vector<float> shortOutput(shortFrames * 2);
        bool shortValid = shortResult && lumaPlayerCoreBackingRead(shortResult, 0, shortOutput.data(), shortFrames) == shortFrames;
        for (size_t i = 0; shortValid && i < shortOutput.size(); ++i)
        {
            shortValid = std::isfinite(shortOutput[i]);
        }
        report.check(id, shortValid && shortProgress.m_ordered && shortProgress.m_last == 100 && lumaPlayerCoreBackingRead(shortResult, shortFrames, shortOutput.data(), 1) == 0,
            "3-second input preserves exact length, finite boundaries, and bounded EOF read");
        lumaPlayerCoreBackingDestroy(shortResult);
        LumaPlayerCoreCSeparationOptions invalidOptions = {sizeof(invalidOptions), 8, 4};
        canceledResult = lumaPlayerCoreSeparateVocalsEx(source.toUtf8().constData(), runtime.constData(), model.constData(),
            32 * 1024 * 1024, &invalidOptions, nullptr, nullptr, error, sizeof(error));
        report.check(id, !canceledResult && QByteArray(error).contains("Invalid"), "window size incompatible with the fixed model is rejected");
        LumaPlayerCoreHandle core = lumaPlayerCoreCreate();
        report.check(id, lumaPlayerCoreBackingApply(core, "different-source", result) != 0,
            "stale/unopened source cannot accept backing");
        std::vector<int16_t> recording(4800, 100);
        lumaPlayerCoreKaraokeInit(core, 48000, 1, 0);
        lumaPlayerCoreKaraokeWritePcm(core, 0, 1000000, 1000, recording.data(), recording.size());
        const QString wrongTarget = directory + "/wrong-source.mp3";
        report.check(id, lumaPlayerCoreKaraokeExport(core, "different-source", wrongTarget.toUtf8().constData(), 1, 0, 0, 100, result, nullptr, nullptr, error, sizeof(error)) != 0 && !QFile::exists(wrongTarget),
            "export rejects backing from another source before creating output");
        lumaPlayerCoreDestroy(core);
        lumaPlayerCoreBackingDestroy(result);
    }
}