#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"

/* 该编译单元只使用C接口，确保API.h的宏分支始终可以被C编译器消费。 */
int pdfReaderCoreCCompileSmoke(void)
{
    PdfReaderCoreCPageInfo info;
    PdfReaderCoreCConfig config;
    pdfReaderCoreDefaultConfig(&config);
    info.width = 0.0;
    info.height = 0.0;
    return (int)info.width;
}
