#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MA_WASAPI_USE_RAW_MODE
#include "KaraokeNativeAudio.h"
#define MA_WASAPI_CAPTURE_PACKET_OBSERVER(user, timestamp, flags, padding, rate) \
    static_cast<KaraokeNativeAudio*>(user)->observeCapturePacket(timestamp, flags, padding, rate)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"