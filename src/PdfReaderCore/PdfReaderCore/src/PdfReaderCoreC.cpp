#include "PdfReaderCoreC.h"

#include "PdfReaderCoreCContext.h"
#include "PdfReaderCoreCResultHelper.h"
#include "CStringManager/CStringManagerAPI.h"

#include <algorithm>
#include <cstring>

PdfReaderCoreAPI PdfReaderCoreHandle pdfReaderCoreCreate(void)
{
    try { return new PdfReaderCoreCContext(); }
    catch (...) { return nullptr; }
}

PdfReaderCoreAPI void pdfReaderCoreDestroy(PdfReaderCoreHandle handle)
{
    delete handle;
}

PdfReaderCoreAPI int32_t pdfReaderCoreInit(PdfReaderCoreHandle handle)
{
    if (!handle) return PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        return handle->core.init(&error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                                         : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultInternalError, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI void pdfReaderCoreDefaultConfig(PdfReaderCoreCConfig* config)
{
    if (!config) { return; }
    const PdfReaderCoreConfig defaults;
    config->maxRenderPixels = defaults.maxRenderPixels;
    config->exportNumberWidth = defaults.exportNumberWidth;
}

PdfReaderCoreAPI int32_t pdfReaderCoreInitWithConfig(PdfReaderCoreHandle handle, const PdfReaderCoreCConfig* config)
{
    if (!handle || !config) { return PdfReaderCoreCResultInvalidParam; }
    try
    {
        PdfReaderCoreConfig value;
        value.maxRenderPixels = config->maxRenderPixels;
        value.exportNumberWidth = config->exportNumberWidth;
        std::string error;
        return handle->core.init(value, &error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess) :
            PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultInvalidParam, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI void pdfReaderCoreUninit(PdfReaderCoreHandle handle)
{
    if (!handle) return;
    try { handle->core.uninit(); handle->lastResult = PdfReaderCoreCResultSuccess; handle->lastError.clear(); }
    catch (...) { handle->lastResult = PdfReaderCoreCResultInternalError; }
}

PdfReaderCoreAPI int32_t pdfReaderCoreOpen(PdfReaderCoreHandle handle, const char* filePath, const char* password)
{
    if (!handle || !filePath || !*filePath) return handle ? PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultInvalidParam, "file path is empty") : PdfReaderCoreCResultInvalidParam;
    try
    {
        if (!handle->core.isInit()) return PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultNotInit, "pdf engine is not initialized");
        std::string error;
        const std::wstring path = CStringManager::Utf8ToUnicode(filePath);
        const bool ok = handle->core.open(path, password ? password : std::string(), &error);
        return ok ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                  : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultOpenFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI void pdfReaderCoreClose(PdfReaderCoreHandle handle)
{
    if (!handle) return;
    try { handle->core.close(); handle->lastResult = PdfReaderCoreCResultSuccess; handle->lastError.clear(); }
    catch (...) { handle->lastResult = PdfReaderCoreCResultInternalError; }
}

PdfReaderCoreAPI int32_t pdfReaderCoreIsOpen(PdfReaderCoreHandle handle)
{
    if (!handle) return 0;
    try { return handle->core.isOpen() ? 1 : 0; }
    catch (...) { return 0; }
}

PdfReaderCoreAPI int32_t pdfReaderCorePageCount(PdfReaderCoreHandle handle)
{
    if (!handle) return 0;
    try { return handle->core.pageCount(); }
    catch (...) { return 0; }
}

PdfReaderCoreAPI size_t pdfReaderCoreGetFilePath(PdfReaderCoreHandle handle, char* buffer, size_t bufferSize)
{
    if (!handle) return 0;
    try
    {
        const std::string path = CStringManager::UnicodeToUtf8(handle->core.filePath());
        const size_t required = path.size() + 1;
        if (buffer && bufferSize)
        {
            const size_t copySize = (std::min)(bufferSize - 1, path.size());
            if (copySize) std::memcpy(buffer, path.data(), copySize);
            buffer[copySize] = 0;
        }
        return required;
    }
    catch (...) { return 0; }
}

PdfReaderCoreAPI int32_t pdfReaderCoreGetPageInfo(PdfReaderCoreHandle handle, int32_t pageIndex, PdfReaderCoreCPageInfo* info)
{
    if (!handle || !info) return handle ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInvalidParam) : PdfReaderCoreCResultInvalidParam;
    try
    {
        PdfReaderCorePageInfo pageInfo;
        std::string error;
        if (!handle->core.pageInfo(pageIndex, &pageInfo, &error))
            return PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultNotOpen, error);
        info->width = pageInfo.width;
        info->height = pageInfo.height;
        return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreRenderPage(PdfReaderCoreHandle handle, int32_t pageIndex,
                                                  int32_t pixelWidth, int32_t pixelHeight,
                                                  unsigned char* pixels, size_t capacity,
                                                  int32_t* outWidth, int32_t* outHeight,
                                                  int32_t* outStride, size_t* outBytes)
{
    if (!handle || !outWidth || !outHeight || !outStride || !outBytes || pixelWidth <= 0 || pixelHeight <= 0)
        return handle ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInvalidParam) : PdfReaderCoreCResultInvalidParam;
    try
    {
        PdfEngineBitmap bitmap;
        std::string error;
        if (!handle->core.renderPage(pageIndex, pixelWidth, pixelHeight, &bitmap, &error))
            return PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultRenderFailed, error);
        *outWidth = bitmap.width;
        *outHeight = bitmap.height;
        *outStride = bitmap.stride;
        *outBytes = bitmap.pixels.size();
        if (!pixels || capacity < bitmap.pixels.size())
            return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultBufferTooSmall);
        if (!bitmap.pixels.empty())
            std::memcpy(pixels, &bitmap.pixels[0], bitmap.pixels.size());
        return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreInsertDocument(PdfReaderCoreHandle handle, const char* filePath,
                                                      const char* password, int32_t insertIndex)
{
    if (!handle || !filePath || !*filePath) return handle ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInvalidParam) : PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        const bool ok = handle->core.insertDocument(CStringManager::Utf8ToUnicode(filePath), password ? password : std::string(), insertIndex, &error);
        return ok ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                  : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultOpenFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreMovePage(PdfReaderCoreHandle handle, int32_t fromIndex, int32_t toIndex)
{
    if (!handle) return PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        return handle->core.movePage(fromIndex, toIndex, &error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                                                                  : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultNotOpen, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreSaveTo(PdfReaderCoreHandle handle, const char* outputFilePath)
{
    if (!handle || !outputFilePath || !*outputFilePath) return handle ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInvalidParam) : PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        return handle->core.saveTo(CStringManager::Utf8ToUnicode(outputFilePath), &error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                                                                                             : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultSaveFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreSaveToMain(PdfReaderCoreHandle handle)
{
    if (!handle) return PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        return handle->core.saveToMain(&error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                                               : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultSaveFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreValidatePageRange(PdfReaderCoreHandle handle, const char* rangeText)
{
    if (!handle || !rangeText) { return PdfReaderCoreCResultInvalidParam; }
    try
    {
        std::string error;
        return handle->core.validatePageRange(rangeText, &error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
            : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultParseFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreSavePageRange(PdfReaderCoreHandle handle, const char* rangeText, const char* outputFilePath)
{
    if (!handle || !rangeText || !outputFilePath || !*outputFilePath)
        return handle ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInvalidParam) : PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        return handle->core.savePageRange(rangeText, CStringManager::Utf8ToUnicode(outputFilePath), &error) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                                                                                                                : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultParseFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI int32_t pdfReaderCoreSaveEachPage(PdfReaderCoreHandle handle, const char* outputDirectory, const char* namePrefix)
{
    return pdfReaderCoreSaveEachPageEx(handle, outputDirectory, namePrefix, 0);
}

PdfReaderCoreAPI int32_t pdfReaderCoreSaveEachPageEx(PdfReaderCoreHandle handle, const char* outputDirectory, const char* namePrefix, int32_t overwrite)
{
    if (!handle || !outputDirectory || !*outputDirectory)
        return handle ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInvalidParam) : PdfReaderCoreCResultInvalidParam;
    try
    {
        std::string error;
        return handle->core.saveEachPage(CStringManager::Utf8ToUnicode(outputDirectory), namePrefix ? namePrefix : std::string(), &error, overwrite != 0) ? PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultSuccess)
                                                                                                                                      : PdfReaderCoreCResultHelper::runError(handle, PdfReaderCoreCResultSaveFailed, error);
    }
    catch (...) { return PdfReaderCoreCResultHelper::runResult(handle, PdfReaderCoreCResultInternalError); }
}

PdfReaderCoreAPI size_t pdfReaderCoreGetLastError(PdfReaderCoreHandle handle, char* buffer, size_t bufferSize)
{
    if (!handle) return 0;
    try
    {
        const std::string error = handle->lastError;
        const size_t required = error.size() + 1;
        if (buffer && bufferSize)
        {
            const size_t copySize = (std::min)(bufferSize - 1, error.size());
            if (copySize) std::memcpy(buffer, error.data(), copySize);
            buffer[copySize] = 0;
        }
        return required;
    }
    catch (...) { return 0; }
}

PdfReaderCoreAPI const char* pdfReaderCoreResultDescription(int32_t result)
{
    switch (result)
    {
    case PdfReaderCoreCResultSuccess: return "success";
    case PdfReaderCoreCResultInvalidParam: return "invalid parameter";
    case PdfReaderCoreCResultNotInit: return "core is not initialized";
    case PdfReaderCoreCResultNotOpen: return "no PDF document is open";
    case PdfReaderCoreCResultOpenFailed: return "failed to open PDF";
    case PdfReaderCoreCResultPasswordRequired: return "PDF password is required or incorrect";
    case PdfReaderCoreCResultSaveFailed: return "failed to save PDF";
    case PdfReaderCoreCResultRenderFailed: return "failed to render PDF page";
    case PdfReaderCoreCResultParseFailed: return "failed to parse page range";
    case PdfReaderCoreCResultBufferTooSmall: return "output bitmap buffer is too small";
    case PdfReaderCoreCResultFileExists: return "output file already exists";
    default: return "internal error";
    }
}