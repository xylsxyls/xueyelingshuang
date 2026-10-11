#pragma once
#include <stdint.h>

/** 文件协议的固定签名，不属于可调配置
*/
class SplitViewerCoreFormat
{
public:
    // PNG文件固定签名
    static const uint8_t kPngSignature[8];
    // PNG结束块类型
    static const uint8_t kPngIend[4];
    // SplitViewer配置块类型
    static const uint8_t kConfigChunk[4];
    // 兼容旧版的尾部配置标记
    static const char kLegacyMarker[35];
};