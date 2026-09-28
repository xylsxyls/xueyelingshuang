#include "PdfReaderCoreCResultHelper.h"
#include "PdfReaderCoreCContext.h"
#include "CStringManager/CStringManagerAPI.h"

int32_t PdfReaderCoreCResultHelper::classifyError(int32_t fallback, const std::string& error)
{
    const std::string lower = CStringManager::MakeLower(error);
    if (lower.find("output file already exists:") == 0)
    {
        return PdfReaderCoreCResultFileExists;
    }
    if (lower.find("no pdf document") != std::string::npos)
        return PdfReaderCoreCResultNotOpen;
    if (lower.find("page index") == 0 || lower.find("insert index") == 0 || lower.find("invalid ") == 0)
        return PdfReaderCoreCResultInvalidParam;
    if (fallback == PdfReaderCoreCResultOpenFailed && lower.find("password") != std::string::npos)
        return PdfReaderCoreCResultPasswordRequired;
    if (fallback == PdfReaderCoreCResultOpenFailed && lower.find("parse") != std::string::npos)
        return PdfReaderCoreCResultParseFailed;
    return fallback;
}

int32_t PdfReaderCoreCResultHelper::runResult(PdfReaderCoreCContext* context, int32_t result)
{
    if (context)
    {
        context->lastResult = result;
        // A status-only failure must not keep details from an earlier request.
        context->lastError.clear();
    }
    return result;
}

int32_t PdfReaderCoreCResultHelper::runError(PdfReaderCoreCContext* context, int32_t result, const std::string& error)
{
    if (context)
    {
        context->lastResult = PdfReaderCoreCResultHelper::classifyError(result, error);
        context->lastError = error;
    }
    return context ? context->lastResult : PdfReaderCoreCResultInvalidParam;
}