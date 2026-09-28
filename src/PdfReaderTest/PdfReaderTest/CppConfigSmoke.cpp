#include "PdfReaderCore/PdfReaderCoreAPI.h"

/** 以默认 C++ API 入口验证 CoreConfig 与 Core 生命周期，不依赖 Qt */
bool PdfReaderCppConfigSmoke()
{
    PdfReaderCoreConfig config;
    if (!config.isValid() || config.m_maxRenderPixels != 64000000 || config.m_exportNumberWidth != 3)
    {
        return false;
    }
    PdfReaderCore core;
    config.m_exportNumberWidth = 2;
    if (!core.init(config) || !core.isInit()) { return false; }
    if (core.init(config)) { return false; }
    core.uninit();
    config.m_exportNumberWidth = 10;
    return !core.init(config) && !core.isInit();
}