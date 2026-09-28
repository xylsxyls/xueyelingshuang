#include "PdfReaderCoreConfig.h"

PdfReaderCoreConfig::PdfReaderCoreConfig() :
m_maxRenderPixels(64000000), m_exportNumberWidth(3)
{

}

bool PdfReaderCoreConfig::isValid() const
{
    return m_maxRenderPixels > 0 && m_maxRenderPixels <= 268435456 &&
        m_exportNumberWidth >= 1 && m_exportNumberWidth <= 9;
}