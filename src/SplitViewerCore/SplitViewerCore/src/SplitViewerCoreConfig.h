#pragma once
#include <stdint.h>
/** Core format constants. */
class SplitViewerCoreConfig
{
public:
    static const uint8_t PngSignature[8];
    static const uint8_t PngIend[4];
    static const uint8_t ConfigChunk[4];
    static const char LegacyMarker[35];
};