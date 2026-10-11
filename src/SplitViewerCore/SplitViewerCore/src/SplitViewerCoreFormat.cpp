#include "SplitViewerCoreFormat.h"

const uint8_t SplitViewerCoreFormat::kPngSignature[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
const uint8_t SplitViewerCoreFormat::kPngIend[4] = {'I', 'E', 'N', 'D'};
const uint8_t SplitViewerCoreFormat::kConfigChunk[4] = {'s', 'v', 'C', 'f'};
const char SplitViewerCoreFormat::kLegacyMarker[35] = "\r\n--SPLITVIEWER_CONFIG_UTF16LE--\r\n";