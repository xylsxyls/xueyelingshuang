#include "PdfReaderCoreConfig.h"

PdfReaderCoreConfig::PdfReaderCoreConfig() :
maxRenderPixels(64000000), exportNumberWidth(3)
{

}

bool PdfReaderCoreConfig::isValid() const
{
    return maxRenderPixels > 0 && maxRenderPixels <= 268435456 &&
        exportNumberWidth >= 1 && exportNumberWidth <= 9;
}