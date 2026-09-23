#pragma once

#include "PdfReaderCoreMacro.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PdfReaderCoreCContext* PdfReaderCoreHandle;

typedef enum PdfReaderCoreCResult
{
    PdfReaderCoreCResultSuccess = 0,
    PdfReaderCoreCResultInvalidParam = 1,
    PdfReaderCoreCResultNotInit = 2,
    PdfReaderCoreCResultNotOpen = 3,
    PdfReaderCoreCResultOpenFailed = 4,
    PdfReaderCoreCResultPasswordRequired = 5,
    PdfReaderCoreCResultSaveFailed = 6,
    PdfReaderCoreCResultRenderFailed = 7,
    PdfReaderCoreCResultParseFailed = 8,
    PdfReaderCoreCResultBufferTooSmall = 9,
    PdfReaderCoreCResultFileExists = 10,
    PdfReaderCoreCResultInternalError = 11
} PdfReaderCoreCResult;

typedef struct PdfReaderCoreCPageInfo
{
    double width;
    double height;
} PdfReaderCoreCPageInfo;

/* 初始化配置：默认值由 DefaultConfig 填充；InitWithConfig 校验、复制，运行期不可更改。 */
typedef struct PdfReaderCoreCConfig
{
    uint64_t maxRenderPixels;
    int32_t exportNumberWidth;
} PdfReaderCoreCConfig;

PdfReaderCoreAPI void pdfReaderCoreDefaultConfig(PdfReaderCoreCConfig* config);
PdfReaderCoreAPI int32_t pdfReaderCoreInitWithConfig(PdfReaderCoreHandle handle, const PdfReaderCoreCConfig* config);

PdfReaderCoreAPI PdfReaderCoreHandle pdfReaderCoreCreate(void);
PdfReaderCoreAPI void pdfReaderCoreDestroy(PdfReaderCoreHandle handle);
PdfReaderCoreAPI int32_t pdfReaderCoreInit(PdfReaderCoreHandle handle);
PdfReaderCoreAPI void pdfReaderCoreUninit(PdfReaderCoreHandle handle);

/* 路径和密码均为UTF-8。 */
PdfReaderCoreAPI int32_t pdfReaderCoreOpen(PdfReaderCoreHandle handle, const char* filePath, const char* password);
PdfReaderCoreAPI void pdfReaderCoreClose(PdfReaderCoreHandle handle);
PdfReaderCoreAPI int32_t pdfReaderCoreIsOpen(PdfReaderCoreHandle handle);
PdfReaderCoreAPI int32_t pdfReaderCorePageCount(PdfReaderCoreHandle handle);
PdfReaderCoreAPI size_t pdfReaderCoreGetFilePath(PdfReaderCoreHandle handle, char* buffer, size_t bufferSize);
PdfReaderCoreAPI int32_t pdfReaderCoreGetPageInfo(PdfReaderCoreHandle handle, int32_t pageIndex, PdfReaderCoreCPageInfo* info);
PdfReaderCoreAPI int32_t pdfReaderCoreRenderPage(PdfReaderCoreHandle handle, int32_t pageIndex,
                                                  int32_t pixelWidth, int32_t pixelHeight,
                                                  unsigned char* pixels, size_t capacity,
                                                  int32_t* outWidth, int32_t* outHeight,
                                                  int32_t* outStride, size_t* outBytes);
PdfReaderCoreAPI int32_t pdfReaderCoreInsertDocument(PdfReaderCoreHandle handle, const char* filePath,
                                                      const char* password, int32_t insertIndex);
PdfReaderCoreAPI int32_t pdfReaderCoreMovePage(PdfReaderCoreHandle handle, int32_t fromIndex, int32_t toIndex);

PdfReaderCoreAPI int32_t pdfReaderCoreSaveTo(PdfReaderCoreHandle handle, const char* outputFilePath);
PdfReaderCoreAPI int32_t pdfReaderCoreSaveToMain(PdfReaderCoreHandle handle);
/* 不写文件，只验证当前文档的页码范围。 */
PdfReaderCoreAPI int32_t pdfReaderCoreValidatePageRange(PdfReaderCoreHandle handle, const char* rangeText);
PdfReaderCoreAPI int32_t pdfReaderCoreSavePageRange(PdfReaderCoreHandle handle, const char* rangeText, const char* outputFilePath);
PdfReaderCoreAPI int32_t pdfReaderCoreSaveEachPage(PdfReaderCoreHandle handle, const char* outputDirectory, const char* namePrefix);
/* 默认不覆盖；调用方确认冲突后传 overwrite=1。所有目标先检查，失败返回 FileExists。 */
PdfReaderCoreAPI int32_t pdfReaderCoreSaveEachPageEx(PdfReaderCoreHandle handle, const char* outputDirectory, const char* namePrefix, int32_t overwrite);

/* 返回所需字节数，包含结尾的NUL；buffer可为空。 */
PdfReaderCoreAPI size_t pdfReaderCoreGetLastError(PdfReaderCoreHandle handle, char* buffer, size_t bufferSize);
PdfReaderCoreAPI const char* pdfReaderCoreResultDescription(int32_t result);

#ifdef __cplusplus
}
#endif