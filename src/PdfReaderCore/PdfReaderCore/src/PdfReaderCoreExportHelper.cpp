#include "PdfReaderCoreExportHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

std::string PdfReaderCoreExportHelper::pageFileName(const std::string& directory,
    const std::string& prefix, int32_t page, int32_t numberWidth)
{
    return directory + prefix + "_" + CStringManager::Format("%0*d", numberWidth, page) + ".pdf";
}

bool PdfReaderCoreExportHelper::savePages(PdfEngine& engine, const std::wstring& outputPath,
    const std::vector<PdfEnginePageRef>& refs, std::string* errorText)
{
    if (outputPath.empty())
    {
        if (errorText) { *errorText = "output path is empty"; }
        return false;
    }
    const std::wstring temporary = outputPath + L".pdfreader-" + CStringManager::Utf8ToUnicode(CSystem::uuid()) + L".tmp";
    if (!engine.savePages(temporary, refs, errorText))
    {
        CSystem::deleteFile(temporary);
        return false;
    }
    if (!CSystem::replaceFile(temporary, outputPath))
    {
        if (errorText)
        {
            *errorText = "failed to replace output PDF; recovered PDF: " + CStringManager::UnicodeToUtf8(temporary);
        }
        return false;
    }
    return true;
}