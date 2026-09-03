#include "VideoTimelineProbe.h"
#include "LoopPlayerConstants.h"
#include "LoopPlayerUtils.h"

namespace LoopPlayer
{
    VideoTimelineInfo::VideoTimelineInfo()
        : isValid(false),
          sampleCount(0),
          result(E_FAIL),
          firstTime(-1),
          secondTime(-1),
          lastTime(-1),
          lastEndTime(0),
          firstDuration(0),
          sourceDuration(0),
          maxGap(0),
          maxGapStart(-1),
          maxGapEnd(-1)
    {
    }

    bool VideoTimelineProbe::Probe(const wchar_t* path, REFERENCE_TIME fallbackFrameDuration, VideoTimelineInfo& info)
    {
        info = VideoTimelineInfo();

        if (!path || !path[0])
        {
            Logf(L"Video timeline probe skipped: empty path");
            return false;
        }

        const REFERENCE_TIME frameDuration = fallbackFrameDuration > 0 ? fallbackFrameDuration : DEFAULT_FRAME_DURATION;
        Logf(L"Video timeline probe begin: path=%s, fallbackFrameDuration=%I64d", path, frameDuration);

        IMFSourceReader* reader = nullptr;
        HRESULT hr = MFCreateSourceReaderFromURL(path, nullptr, &reader);
        info.result = hr;
        if (FAILED(hr))
        {
            Logf(L"MFCreateSourceReaderFromURL failed: 0x%08X", static_cast<unsigned int>(hr));
            return false;
        }

        hr = reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
        Logf(L"SourceReader deselect all streams returned 0x%08X", static_cast<unsigned int>(hr));

        hr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        if (FAILED(hr))
        {
            info.result = hr;
            Logf(L"SourceReader select first video stream failed: 0x%08X", static_cast<unsigned int>(hr));
            SafeRelease(reader);
            return false;
        }
        Logf(L"SourceReader select first video stream succeeded");

        IMFMediaType* nativeType = nullptr;
        hr = reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeType);
        Logf(L"SourceReader GetNativeMediaType returned 0x%08X", static_cast<unsigned int>(hr));
        if (SUCCEEDED(hr) && nativeType)
        {
            HRESULT typeHr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, nativeType);
            Logf(L"SourceReader SetCurrentMediaType(native) returned 0x%08X", static_cast<unsigned int>(typeHr));
        }
        SafeRelease(nativeType);

        PROPVARIANT sourceDurationValue;
        PropVariantInit(&sourceDurationValue);
        hr = reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &sourceDurationValue);
        LONGLONG sourceDuration = 0;
        if (SUCCEEDED(hr) && ReadInt64PropVariant(sourceDurationValue, sourceDuration))
        {
            info.sourceDuration = sourceDuration;
        }
        Logf(L"SourceReader presentation duration: hr=0x%08X, vt=%u, duration=%s (%I64d)",
             static_cast<unsigned int>(hr),
             static_cast<unsigned int>(sourceDurationValue.vt),
             FormatTime(info.sourceDuration).c_str(),
             info.sourceDuration);
        PropVariantClear(&sourceDurationValue);

        bool havePreviousTime = false;
        REFERENCE_TIME previousTime = 0;

        for (;;)
        {
            DWORD actualStream = 0;
            DWORD flags = 0;
            LONGLONG timestamp = 0;
            IMFSample* sample = nullptr;

            hr = reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                    0,
                                    &actualStream,
                                    &flags,
                                    &timestamp,
                                    &sample);
            if (FAILED(hr))
            {
                info.result = hr;
                Logf(L"SourceReader ReadSample failed: 0x%08X, sampleCount=%lu", static_cast<unsigned int>(hr), info.sampleCount);
                SafeRelease(sample);
                break;
            }

            if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0)
            {
                Logf(L"SourceReader video media type changed at sampleCount=%lu", info.sampleCount);
            }

            if ((flags & MF_SOURCE_READERF_ERROR) != 0)
            {
                info.result = E_FAIL;
                Logf(L"SourceReader returned error flag at sampleCount=%lu, flags=0x%08X", info.sampleCount, flags);
                SafeRelease(sample);
                break;
            }

            if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0)
            {
                info.result = S_OK;
                Logf(L"SourceReader reached end of video stream at sampleCount=%lu", info.sampleCount);
                SafeRelease(sample);
                break;
            }

            if (!sample)
            {
                if ((flags & MF_SOURCE_READERF_STREAMTICK) != 0)
                {
                    Logf(L"SourceReader stream tick: timestamp=%s (%I64d), flags=0x%08X",
                         FormatTime(timestamp).c_str(),
                         timestamp,
                         flags);
                }
                continue;
            }

            LONGLONG sampleTime = timestamp;
            LONGLONG explicitSampleTime = 0;
            HRESULT timeHr = sample->GetSampleTime(&explicitSampleTime);
            if (SUCCEEDED(timeHr))
            {
                sampleTime = explicitSampleTime;
            }

            LONGLONG sampleDuration = frameDuration;
            HRESULT durationHr = sample->GetSampleDuration(&sampleDuration);
            if (FAILED(durationHr) || sampleDuration <= 0)
            {
                sampleDuration = frameDuration;
            }

            const REFERENCE_TIME startTime = sampleTime;
            const REFERENCE_TIME endTime = startTime + sampleDuration;
            if (info.sampleCount == 0)
            {
                info.firstTime = startTime;
                info.firstDuration = sampleDuration;
            }
            else if (info.sampleCount == 1)
            {
                info.secondTime = startTime;
            }

            if (havePreviousTime)
            {
                const REFERENCE_TIME gap = startTime - previousTime;
                if (gap > info.maxGap)
                {
                    info.maxGap = gap;
                    info.maxGapStart = previousTime;
                    info.maxGapEnd = startTime;
                }
                if (gap > frameDuration * 15)
                {
                    Logf(L"Video timeline large gap: prev=%s (%I64d), current=%s (%I64d), gap=%s (%I64d), sampleIndex=%lu",
                         FormatTime(previousTime).c_str(),
                         previousTime,
                         FormatTime(startTime).c_str(),
                         startTime,
                         FormatTime(gap).c_str(),
                         gap,
                         info.sampleCount);
                }
            }

            previousTime = startTime;
            havePreviousTime = true;
            info.lastTime = startTime;
            if (endTime > info.lastEndTime)
            {
                info.lastEndTime = endTime;
            }
            ++info.sampleCount;

            if (info.sampleCount <= 5 || (info.sampleCount % 300) == 0)
            {
                Logf(L"Video sample timeline: index=%lu, time=%s (%I64d), duration=%s (%I64d), end=%s (%I64d), timeHr=0x%08X, durationHr=0x%08X",
                     info.sampleCount,
                     FormatTime(startTime).c_str(),
                     startTime,
                     FormatTime(sampleDuration).c_str(),
                     sampleDuration,
                     FormatTime(endTime).c_str(),
                     endTime,
                     static_cast<unsigned int>(timeHr),
                     static_cast<unsigned int>(durationHr));
            }

            SafeRelease(sample);
        }

        SafeRelease(reader);

        info.isValid = SUCCEEDED(info.result) && info.sampleCount > 0 && info.lastEndTime > 0;
        Logf(L"Video timeline probe end: valid=%d, result=0x%08X, samples=%lu, sourceDuration=%s (%I64d), first=%s (%I64d), second=%s (%I64d), firstDuration=%s (%I64d), last=%s (%I64d), lastEnd=%s (%I64d), maxGap=%s (%I64d), maxGapStart=%s (%I64d), maxGapEnd=%s (%I64d)",
             info.isValid ? 1 : 0,
             static_cast<unsigned int>(info.result),
             info.sampleCount,
             FormatTime(info.sourceDuration).c_str(),
             info.sourceDuration,
             FormatTime(info.firstTime).c_str(),
             info.firstTime,
             FormatTime(info.secondTime).c_str(),
             info.secondTime,
             FormatTime(info.firstDuration).c_str(),
             info.firstDuration,
             FormatTime(info.lastTime).c_str(),
             info.lastTime,
             FormatTime(info.lastEndTime).c_str(),
             info.lastEndTime,
             FormatTime(info.maxGap).c_str(),
             info.maxGap,
             FormatTime(info.maxGapStart).c_str(),
             info.maxGapStart,
             FormatTime(info.maxGapEnd).c_str(),
             info.maxGapEnd);
        return info.isValid;
    }
}