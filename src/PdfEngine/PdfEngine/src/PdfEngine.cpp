#include "PdfEngine.h"

#include "../internal/PdfEngineHelper.h"
#include "../internal/PdfEngineInstance.h"
#include "../internal/PdfEngineFileWriter.h"

#include <CSystem/CSystemAPI.h>

#include <cstdio>
#include <memory>
#include <new>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include <pdfium/fpdf_edit.h>
#include <pdfium/fpdf_ppo.h>
#include <pdfium/fpdf_save.h>
#include <pdfium/fpdfview.h>

#ifdef _MSC_VER
#pragma comment(lib, "pdfium.lib")
#endif

PdfEngine::PdfEngine() :
m_isInit(false)
{

}

PdfEngine::~PdfEngine()
{
    uninit();
}

bool PdfEngine::init(std::string* errorText)
{
    PdfEngineHelper::setErrorText(errorText, "");
    if (m_isInit)
    {
        return true;
    }

    PdfEngineInstance::instance().addInitRef();
    m_isInit = true;
    return true;
}

void PdfEngine::uninit()
{
    if (!m_isInit)
    {
        return;
    }

    m_isInit = false;
    PdfEngineInstance::instance().releaseInitRef();
}

bool PdfEngine::isInit() const
{
    return m_isInit;
}

bool PdfEngine::savePages(const std::wstring& outputFilePath,
                          const std::vector<PdfEnginePageRef>& pages,
                          std::string* errorText)
{
    PdfEngineHelper::setErrorText(errorText, "");

    if (!m_isInit)
    {
        PdfEngineHelper::setErrorText(errorText, "PdfEngine is not initialized");
        return false;
    }

    if (outputFilePath.empty())
    {
        PdfEngineHelper::setErrorText(errorText, "output pdf file path is empty");
        return false;
    }

    if (pages.empty())
    {
        PdfEngineHelper::setErrorText(errorText, "no pdf pages to save");
        return false;
    }

    FPDF_DOCUMENT outputDocument = FPDF_CreateNewDocument();
    if (outputDocument == nullptr)
    {
        PdfEngineHelper::setErrorText(errorText, PdfEngineHelper::pdfiumErrorText());
        return false;
    }

    bool imported = true;
    for (size_t i = 0; i < pages.size(); ++i)
    {
        const PdfEnginePageRef& pageRef = pages[i];
        if (pageRef.document == nullptr || !pageRef.document->isOpen())
        {
            imported = false;
            PdfEngineHelper::setErrorText(errorText, "page source document is invalid");
            break;
        }

        if (pageRef.pageIndex < 0 || pageRef.pageIndex >= pageRef.document->pageCount())
        {
            imported = false;
            PdfEngineHelper::setErrorText(errorText, "page index is out of range");
            break;
        }

        FPDF_DOCUMENT sourceDocument = reinterpret_cast<FPDF_DOCUMENT>(pageRef.document->nativeDocumentForPdfEngine());
        int pageIndex = pageRef.pageIndex;
        if (!FPDF_ImportPagesByIndex(outputDocument, sourceDocument, &pageIndex, 1, static_cast<int>(i)))
        {
            imported = false;
            PdfEngineHelper::setErrorText(errorText, PdfEngineHelper::pdfiumErrorText());
            break;
        }

        if (i == 0)
        {
            FPDF_CopyViewerPreferences(outputDocument, sourceDocument);
        }
    }

    if (!imported)
    {
        FPDF_CloseDocument(outputDocument);
        return false;
    }

    std::string fileError;
    FILE* outputFile = CSystem::openBinaryOutputFile(outputFilePath, &fileError);
    if (outputFile == nullptr)
    {
        FPDF_CloseDocument(outputDocument);
        PdfEngineHelper::setErrorText(errorText, fileError.empty() ? "open output pdf file failed" : fileError);
        return false;
    }

    std::unique_ptr<PdfEngineFileWriter> writer(new (std::nothrow) PdfEngineFileWriter(outputFile));
    if (writer.get() == nullptr || writer->nativeWriter() == nullptr)
    {
        fclose(outputFile);
        FPDF_CloseDocument(outputDocument);
        CSystem::deleteFile(outputFilePath);
        PdfEngineHelper::setErrorText(errorText, "create pdf file writer failed");
        return false;
    }

    bool saved = FPDF_SaveAsCopy(outputDocument, writer->nativeWriter(), FPDF_NO_INCREMENTAL | FPDF_REMOVE_SECURITY) ? true : false;
    std::string saveError = saved ? "" : PdfEngineHelper::pdfiumErrorText();
    fclose(outputFile);
    FPDF_CloseDocument(outputDocument);

    if (!saved || writer->failed())
    {
        CSystem::deleteFile(outputFilePath);
        PdfEngineHelper::setErrorText(errorText, writer->failed() ? "write output pdf file failed" : saveError);
        return false;
    }

    return true;
}

bool PdfEngine::parsePageRanges(const std::string& rangeText,
                                int32_t pageCount,
                                std::vector<int32_t>* pageIndexes,
                                std::string* errorText)
{
    return PdfEngineHelper::parsePageRanges(rangeText, pageCount, pageIndexes, errorText);
}

// int main(int argc, char* argv[])
// {
//     if (argc < 3)
//     {
//         return 1;
//     }
//
//     PdfEngine engine;
//     std::string error;
//     if (!engine.init(&error))
//     {
//         return 2;
//     }
//
//     std::wstring inputPath;
//     std::wstring outputPath;
// #ifdef _WIN32
//     int wideArgc = 0;
//     LPWSTR* wideArgv = CommandLineToArgvW(GetCommandLineW(), &wideArgc);
//     if (wideArgv != nullptr && wideArgc >= 3)
//     {
//         inputPath = wideArgv[1];
//         outputPath = wideArgv[2];
//         LocalFree(wideArgv);
//     }
// #else
//     inputPath = CStringManager::Utf8ToUnicode(argv[1]);
//     outputPath = CStringManager::Utf8ToUnicode(argv[2]);
// #endif
//
//     PdfDocument document;
//     if (!document.open(inputPath, &error))
//     {
//         return 3;
//     }
//
//     PdfEnginePageSize pageSize;
//     if (!document.getPageSize(0, &pageSize, &error))
//     {
//         return 4;
//     }
//
//     PdfEngineBitmap bitmap;
//     if (!document.renderPageToBgra(0, 300, 420, &bitmap, &error))
//     {
//         return 5;
//     }
//
//     std::vector<PdfEnginePageRef> pages;
//     pages.push_back(PdfEnginePageRef(&document, 0));
//     if (!engine.savePages(outputPath, pages, &error))
//     {
//         return 6;
//     }
//
//     document.close();
//     engine.uninit();
//     return 0;
// }