#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mmreg.h>
#include <stdint.h>
#include <stddef.h>
#include <cstring>
#include <string>

template<class T> struct FFmpegCppComPtr
{
    T* p;
    FFmpegCppComPtr() : p(nullptr) {}
    ~FFmpegCppComPtr() { if (p) p->Release(); }
    T** address() { return &p; }
    T* operator->() { return p; }
};

struct FFmpegCppNativeMediaWriterState
{
    IMFSinkWriter* m_writer;
    DWORD m_audio, m_video;
    HRESULT m_status;
    bool m_com, m_mf;
    const char* m_stage;
    FFmpegCppNativeMediaWriterState() : m_writer(nullptr), m_audio(0), m_video(0), m_status(S_OK), m_com(false), m_mf(false), m_stage("initialize") {}
    ~FFmpegCppNativeMediaWriterState()
    {
        if (m_writer) m_writer->Release();
        if (m_mf) MFShutdown();
        if (m_com) CoUninitialize();
    }
    bool write(DWORD index, const void* data, size_t bytes, int64_t time, int64_t duration)
    {
        if (!m_writer || !data || bytes == 0 || bytes > MAXDWORD) { m_status = E_INVALIDARG; return false; }
        FFmpegCppComPtr<IMFSample> sample;
        FFmpegCppComPtr<IMFMediaBuffer> buffer;
        m_status = MFCreateSample(sample.address());
        if (SUCCEEDED(m_status)) m_status = MFCreateMemoryBuffer(static_cast<DWORD>(bytes), buffer.address());
        BYTE* target = nullptr;
        if (SUCCEEDED(m_status)) m_status = buffer->Lock(&target, nullptr, nullptr);
        if (SUCCEEDED(m_status))
        {
            std::memcpy(target, data, bytes);
            m_status = buffer->Unlock();
        }
        if (SUCCEEDED(m_status)) m_status = buffer->SetCurrentLength(static_cast<DWORD>(bytes));
        if (SUCCEEDED(m_status)) m_status = sample->AddBuffer(buffer.p);
        if (SUCCEEDED(m_status)) m_status = sample->SetSampleTime(time);
        if (SUCCEEDED(m_status)) m_status = sample->SetSampleDuration(duration);
        if (SUCCEEDED(m_status)) m_status = m_writer->WriteSample(index, sample.p);
        return SUCCEEDED(m_status);
    }
};


/** Windows encoder fallback when the supplied FFmpeg build lacks MP3/AAC/H.264 encoders. */
class FFmpegCppNativeMediaWriter
{
public:
    FFmpegCppNativeMediaWriter();
    ~FFmpegCppNativeMediaWriter();
    bool open(const std::string& path, bool video, int width, int height);
    bool audio(const int16_t* data, size_t frames, int64_t time);
    bool video(const unsigned char* data, size_t bytes, int64_t time, int64_t duration);
    bool finish();
    std::string error() const;
private:
    FFmpegCppNativeMediaWriter(const FFmpegCppNativeMediaWriter&);
    FFmpegCppNativeMediaWriter& operator=(const FFmpegCppNativeMediaWriter&);
    FFmpegCppNativeMediaWriterState* m_impl;
};
#endif