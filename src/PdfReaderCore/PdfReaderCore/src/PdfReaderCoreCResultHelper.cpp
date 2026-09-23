#include "PdfReaderCoreCResultHelper.h"
#include "PdfReaderCoreCContext.h"
#include "CStringManager/CStringManagerAPI.h"

int32_t PdfReaderCoreCResultHelper::classifyError(int32_t fallback, const std::string& error)
{
    const std::string lower = CStringManager::MakeLower(error);
    if (lower.find("already exists") != std::string::npos)
    {
        return PdfReaderCoreCResultFileExists;
    }
    if (lower.find("no pdf document") != std::string::npos)
        return PdfReaderCoreCResultNotOpen;
    if (lower.find("index") != std::string::npos || lower.find("parameter") != std::string::npos)
        return PdfReaderCoreCResultInvalidParam;
    if (lower.find("password") != std::string::npos)
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
        if (result == PdfReaderCoreCResultSuccess) context->lastError.clear();
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