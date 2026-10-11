#include "KaraokeMicrophoneControls.h"
uint32_t KaraokeMicrophoneControls::encode(int32_t gainPercent, bool muted, bool monitoring)
{
    return static_cast<uint32_t>(gainPercent) |
        (muted ? 1u << 13 : 0u) | (monitoring ? 1u << 14 : 0u);
}

int32_t KaraokeMicrophoneControls::gain(uint32_t controls)
{
    return static_cast<int32_t>(controls & 0x1fffu);
}

bool KaraokeMicrophoneControls::muted(uint32_t controls)
{
    return (controls & (1u << 13)) != 0;
}

bool KaraokeMicrophoneControls::monitoring(uint32_t controls)
{
    return (controls & (1u << 14)) != 0;
}