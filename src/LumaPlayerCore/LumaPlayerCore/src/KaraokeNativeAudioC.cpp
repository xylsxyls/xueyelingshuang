#include "KaraokeNativeAudio.h"

LumaPlayerCoreCMicrophone lumaPlayerCoreMicrophoneCreate(void)
{
    try
    {
        return new KaraokeNativeAudio();
    }
    catch (...)
    {
        return nullptr;
    }
}

void lumaPlayerCoreMicrophoneDestroy(LumaPlayerCoreCMicrophone microphone)
{
    delete static_cast<KaraokeNativeAudio*>(microphone);
}

int32_t lumaPlayerCoreMicrophoneEnumerate(LumaPlayerCoreCMicrophoneNameCallback callback, void* userData)
{
    try
    {
        return KaraokeNativeAudio::enumerate(callback, userData);
    }
    catch (...)
    {
        return LumaPlayerCoreCResultInternalError;
    }
}

int32_t lumaPlayerCoreMicrophonePrepare(LumaPlayerCoreCMicrophone microphone,
    int32_t index, int32_t queueMs, int32_t periodMs)
{
    if (microphone == nullptr)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    try
    {
        return static_cast<KaraokeNativeAudio*>(microphone)->prepare(index, queueMs, periodMs);
    }
    catch (...)
    {
        return LumaPlayerCoreCResultInternalError;
    }
}

int32_t lumaPlayerCoreMicrophoneStart(LumaPlayerCoreCMicrophone microphone)
{
    if (microphone == nullptr)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    try
    {
        return static_cast<KaraokeNativeAudio*>(microphone)->start();
    }
    catch (...)
    {
        return LumaPlayerCoreCResultInternalError;
    }
}

void lumaPlayerCoreMicrophoneStop(LumaPlayerCoreCMicrophone microphone)
{
    if (microphone != nullptr)
    {
        static_cast<KaraokeNativeAudio*>(microphone)->stop();
    }
}

int32_t lumaPlayerCoreMicrophoneControls(LumaPlayerCoreCMicrophone microphone,
    int32_t gain, int32_t muted, int32_t monitoring)
{
    return microphone != nullptr ? static_cast<KaraokeNativeAudio*>(microphone)->controls(gain, muted != 0, monitoring != 0)
                                 : LumaPlayerCoreCResultInvalidParam;
}

size_t lumaPlayerCoreMicrophoneRead(LumaPlayerCoreCMicrophone microphone,
    int16_t* output, size_t capacity, int32_t* remainingMs)
{
    return microphone != nullptr ? static_cast<KaraokeNativeAudio*>(microphone)->read(output, capacity, remainingMs) : 0;
}

int32_t lumaPlayerCoreMicrophoneStats(LumaPlayerCoreCMicrophone microphone,
    LumaPlayerCoreCMicrophoneStats* value)
{
    if (microphone == nullptr || value == nullptr)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    static_cast<KaraokeNativeAudio*>(microphone)->stats(value);
    return 0;
}