#include "PdfDocument.h"

#include "../internal/PdfEngineHelper.h"
#include "../internal/PdfEngineInstance.h"

#include <CStringManager/CStringManagerAPI.h>

#include <cstring>

#include <pdfium/fpdfview.h>

#ifdef _MSC_VER
#pragma comment(lib, "pdfium.lib")
#endif

PdfDocument::PdfDocument() :
m_document(nullptr),
m_pageCount(0)
{

}

PdfDocument::~PdfDocument()
{
    close();
}

bool PdfDocument::open(const std::wstring& filePath, const std::string& password, std::string* errorText)
{
    close();
    PdfEngineHelper::setErrorText(errorText, "");

    if (!PdfEngineInstance::instance().isPdfiumInitialized())
    {
        PdfEngineHelper::setErrorText(errorText, "PdfEngine is not initialized");
        return false;
    }

    if (filePath.empty())
    {
        PdfEngineHelper::setErrorText(errorText, "pdf file path is empty");
        return false;
    }

    std::string utf8Path = CStringManager::UnicodeToUtf8(filePath);
    if (utf8Path.empty())
    {
        PdfEngineHelper::setErrorText(errorText, "convert pdf file path to utf8 failed");
        return false;
    }

    FPDF_DOCUMENT document = FPDF_LoadDocument(utf8Path.c_str(), password.empty() ? nullptr : password.c_str());
    if (document == nullptr)
    {
        PdfEngineHelper::setErrorText(errorText, PdfEngineHelper::pdfiumErrorText());
        return false;
    }

    m_document = document;
    m_filePath = filePath;
    m_pageCount = FPDF_GetPageCount(document);
    return true;
}

bool PdfDocument::open(const std::wstring& filePath, std::string* errorText)
{
    return open(filePath, std::string(), errorText);
}

void PdfDocument::close()
{
    if (m_document != nullptr)
    {
        FPDF_CloseDocument(reinterpret_cast<FPDF_DOCUMENT>(m_document));
    }

    m_document = nullptr;
    m_pageCount = 0;
    m_filePath.clear();
}

bool PdfDocument::isOpen() const
{
    return m_document != nullptr;
}

std::wstring PdfDocument::filePath() const
{
    return m_filePath;
}

int32_t PdfDocument::pageCount() const
{
    return m_pageCount;
}

bool PdfDocument::getPageSize(int32_t pageIndex, PdfEnginePageSize* pageSize, std::string* errorText) const
{
    PdfEngineHelper::setErrorText(errorText, "");
    if (pageSize == nullptr)
    {
        PdfEngineHelper::setErrorText(errorText, "page size output is null");
        return false;
    }

    pageSize->width = 0.0;
    pageSize->height = 0.0;

    if (m_document == nullptr || pageIndex < 0 || pageIndex >= m_pageCount)
    {
        PdfEngineHelper::setErrorText(errorText, "page index is out of range");
        return false;
    }

    double width = 0.0;
    double height = 0.0;
    if (!FPDF_GetPageSizeByIndex(reinterpret_cast<FPDF_DOCUMENT>(m_document), pageIndex, &width, &height))
    {
        PdfEngineHelper::setErrorText(errorText, PdfEngineHelper::pdfiumErrorText());
        return false;
    }

    pageSize->width = width;
    pageSize->height = height;
    return true;
}

bool PdfDocument::renderPageToBgra(int32_t pageIndex,
                                   int32_t pixelWidth,
                                   int32_t pixelHeight,
                                   PdfEngineBitmap* bitmap,
                                   std::string* errorText) const
{
    PdfEngineHelper::setErrorText(errorText, "");
    if (bitmap == nullptr)
    {
        PdfEngineHelper::setErrorText(errorText, "bitmap output is null");
        return false;
    }

    bitmap->clear();
    if (m_document == nullptr || pageIndex < 0 || pageIndex >= m_pageCount || pixelWidth <= 0 || pixelHeight <= 0)
    {
        PdfEngineHelper::setErrorText(errorText, "render page arguments are invalid");
        return false;
    }

    FPDF_PAGE page = FPDF_LoadPage(reinterpret_cast<FPDF_DOCUMENT>(m_document), pageIndex);
    if (page == nullptr)
    {
        PdfEngineHelper::setErrorText(errorText, PdfEngineHelper::pdfiumErrorText());
        return false;
    }

    FPDF_BITMAP nativeBitmap = FPDFBitmap_Create(pixelWidth, pixelHeight, 1);
    if (nativeBitmap == nullptr)
    {
        FPDF_ClosePage(page);
        PdfEngineHelper::setErrorText(errorText, "create pdf bitmap failed");
        return false;
    }

    FPDFBitmap_FillRect(nativeBitmap, 0, 0, pixelWidth, pixelHeight, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(nativeBitmap,
                          page,
                          0,
                          0,
                          pixelWidth,
                          pixelHeight,
                          0,
                          FPDF_ANNOT | FPDF_LCD_TEXT);

    int stride = FPDFBitmap_GetStride(nativeBitmap);
    void* buffer = FPDFBitmap_GetBuffer(nativeBitmap);
    if (stride <= 0 || buffer == nullptr)
    {
        FPDFBitmap_Destroy(nativeBitmap);
        FPDF_ClosePage(page);
        PdfEngineHelper::setErrorText(errorText, "pdf bitmap buffer is invalid");
        return false;
    }

    bitmap->width = pixelWidth;
    bitmap->height = pixelHeight;
    bitmap->stride = stride;
    bitmap->pixels.resize(static_cast<size_t>(stride) * static_cast<size_t>(pixelHeight));
    memcpy(&bitmap->pixels[0], buffer, bitmap->pixels.size());

    FPDFBitmap_Destroy(nativeBitmap);
    FPDF_ClosePage(page);
    return true;
}

void* PdfDocument::nativeDocumentForPdfEngine() const
{
    return nativeDocument();
}

void* PdfDocument::nativeDocument() const
{
    return m_document;
}