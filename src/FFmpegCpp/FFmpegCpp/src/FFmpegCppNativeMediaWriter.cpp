#include "FFmpegCppNativeMediaWriter.h"
#ifdef _WIN32
#include <sstream>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

FFmpegCppNativeMediaWriter::FFmpegCppNativeMediaWriter() : m_impl(new FFmpegCppNativeMediaWriterState) {}
FFmpegCppNativeMediaWriter::~FFmpegCppNativeMediaWriter() { delete m_impl; }

bool FFmpegCppNativeMediaWriter::open(const std::string& path, bool hasVideo, int width, int height)
{
    if (m_impl->m_writer || path.empty() || (hasVideo && (width <= 0 || height <= 0 || width % 2 || height % 2))) return false;
    HRESULT& hr = m_impl->m_status;
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return false;
    m_impl->m_com = true;
    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return false;
    m_impl->m_mf = true;
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, nullptr, 0);
    if (length <= 0) { hr = E_INVALIDARG; return false; }
    std::wstring wide(length, 0);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.c_str(), -1, &wide[0], length);
    FFmpegCppComPtr<IMFAttributes> attrs;
    hr = MFCreateAttributes(attrs.address(), 2);
    if (SUCCEEDED(hr)) hr = attrs->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);
    if (SUCCEEDED(hr)) hr = attrs->SetGUID(MF_TRANSCODE_CONTAINERTYPE, hasVideo ? MFTranscodeContainerType_MPEG4 : MFTranscodeContainerType_MP3);
    if (SUCCEEDED(hr)) hr = MFCreateSinkWriterFromURL(wide.c_str(), nullptr, attrs.p, &m_impl->m_writer);
    if (FAILED(hr)) return false;
    FFmpegCppComPtr<IMFMediaType> audioOut, audioIn;
#define KARAOKE_MF(expression) do { m_impl->m_stage = #expression; hr = (expression); if (FAILED(hr)) return false; } while (0)
    KARAOKE_MF(MFCreateMediaType(audioOut.address()));
    KARAOKE_MF(audioOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
    KARAOKE_MF(audioOut->SetGUID(MF_MT_SUBTYPE, hasVideo ? MFAudioFormat_AAC : MFAudioFormat_MP3));
    KARAOKE_MF(audioOut->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2));
    KARAOKE_MF(audioOut->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000));
    if (hasVideo) KARAOKE_MF(audioOut->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
    else
    {
        MPEGLAYER3WAVEFORMAT format = {};
        format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
        format.wfx.nChannels = 2;
        format.wfx.nSamplesPerSec = 48000;
        format.wfx.nAvgBytesPerSec = 24000;
        format.wfx.nBlockAlign = 1;
        format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
        format.wID = MPEGLAYER3_ID_MPEG;
        format.fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
        format.nBlockSize = 576;
        format.nFramesPerBlock = 1;
        KARAOKE_MF(MFInitMediaTypeFromWaveFormatEx(audioOut.p, &format.wfx, sizeof(format)));
    }
    KARAOKE_MF(audioOut->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 24000));
    if (hasVideo) KARAOKE_MF(audioOut->SetUINT32(MF_MT_AAC_PAYLOAD_TYPE, 0));
    KARAOKE_MF(m_impl->m_writer->AddStream(audioOut.p, &m_impl->m_audio));
    KARAOKE_MF(MFCreateMediaType(audioIn.address()));
    KARAOKE_MF(audioIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
    KARAOKE_MF(audioIn->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
    KARAOKE_MF(audioIn->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2));
    KARAOKE_MF(audioIn->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000));
    KARAOKE_MF(audioIn->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16));
    KARAOKE_MF(audioIn->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4));
    KARAOKE_MF(audioIn->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000));
    KARAOKE_MF(m_impl->m_writer->SetInputMediaType(m_impl->m_audio, audioIn.p, nullptr));
    if (hasVideo)
    {
        FFmpegCppComPtr<IMFMediaType> videoOut, videoIn;
        KARAOKE_MF(MFCreateMediaType(videoOut.address()));
        KARAOKE_MF(videoOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        KARAOKE_MF(videoOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
        KARAOKE_MF(videoOut->SetUINT32(MF_MT_AVG_BITRATE, 8000000));
        KARAOKE_MF(videoOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
        KARAOKE_MF(MFSetAttributeSize(videoOut.p, MF_MT_FRAME_SIZE, width, height));
        KARAOKE_MF(MFSetAttributeRatio(videoOut.p, MF_MT_FRAME_RATE, 30, 1));
        KARAOKE_MF(MFSetAttributeRatio(videoOut.p, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
        KARAOKE_MF(m_impl->m_writer->AddStream(videoOut.p, &m_impl->m_video));
        KARAOKE_MF(MFCreateMediaType(videoIn.address()));
        KARAOKE_MF(videoIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        KARAOKE_MF(videoIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
        KARAOKE_MF(videoIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
        KARAOKE_MF(MFSetAttributeSize(videoIn.p, MF_MT_FRAME_SIZE, width, height));
        KARAOKE_MF(MFSetAttributeRatio(videoIn.p, MF_MT_FRAME_RATE, 30, 1));
        KARAOKE_MF(MFSetAttributeRatio(videoIn.p, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
        KARAOKE_MF(videoIn->SetUINT32(MF_MT_DEFAULT_STRIDE, static_cast<UINT32>(width * 4)));
        KARAOKE_MF(m_impl->m_writer->SetInputMediaType(m_impl->m_video, videoIn.p, nullptr));
    }
    KARAOKE_MF(m_impl->m_writer->BeginWriting());
    return true;
#undef KARAOKE_MF
}

bool FFmpegCppNativeMediaWriter::audio(const int16_t* data, size_t frames, int64_t time)
{
    return frames <= MAXDWORD / 4 && m_impl->write(m_impl->m_audio, data, frames * 4, time, frames * 10000000LL / 48000);
}
bool FFmpegCppNativeMediaWriter::video(const unsigned char* data, size_t bytes, int64_t time, int64_t duration)
{
    return m_impl->write(m_impl->m_video, data, bytes, time, duration);
}
bool FFmpegCppNativeMediaWriter::finish()
{
    if (!m_impl->m_writer) return false;
    m_impl->m_status = m_impl->m_writer->Finalize();
    return SUCCEEDED(m_impl->m_status);
}
std::string FFmpegCppNativeMediaWriter::error() const
{
    std::ostringstream text;
    text << m_impl->m_stage << ": system media encoder HRESULT=0x" << std::hex << static_cast<uint32_t>(m_impl->m_status);
    return text.str();
}
#endif