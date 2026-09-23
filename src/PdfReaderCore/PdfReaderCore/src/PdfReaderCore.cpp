#include "PdfReaderCore.h"
#include "PdfReaderCoreExportHelper.h"

#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

#include <algorithm>
#include <cstdio>
#include <utility>

PdfReaderCorePageInfo::PdfReaderCorePageInfo()
    : width(0.0), height(0.0)
{
}

PdfReaderCore::PdfReaderCore()
    : m_isInit(false)
{
}

PdfReaderCore::~PdfReaderCore()
{
    uninit();
}

bool PdfReaderCore::init(std::string* errorText)
{
    if (m_isInit)
    {
        return true;
    }
    return init(PdfReaderCoreConfig(), errorText);
}

bool PdfReaderCore::init(const PdfReaderCoreConfig& config, std::string* errorText)
{
    if (m_isInit || !config.isValid())
    {
        const std::string error = m_isInit ? "uninit before changing configuration" : "invalid core configuration";
        setError(error);
        if (errorText) { *errorText = error; }
        return false;
    }
    std::string error;
    if (!m_engine.init(&error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    m_config = config;
    m_isInit = true;
    m_lastError.clear();
    return true;
}

void PdfReaderCore::uninit()
{
    close();
    if (m_isInit)
    {
        m_engine.uninit();
        m_isInit = false;
    }
}

bool PdfReaderCore::isInit() const
{
    return m_isInit && m_engine.isInit();
}

PdfDocument* PdfReaderCore::openDocument(const std::wstring& filePath,
                                         const std::string& password,
                                         std::string* errorText)
{
    if (filePath.empty())
    {
        if (errorText) *errorText = "file path is empty";
        return nullptr;
    }
    if (!CSystem::fileExists(filePath))
    {
        if (errorText) *errorText = "file does not exist";
        return nullptr;
    }
    std::unique_ptr<PdfDocument> document(new PdfDocument());
    std::string error;
    if (!document->open(filePath, password, &error))
    {
        if (errorText) *errorText = error;
        return nullptr;
    }
    PdfDocument* result = document.get();
    m_documents.push_back(std::move(document));
    return result;
}

bool PdfReaderCore::open(const std::wstring& filePath,
                         const std::string& password,
                         std::string* errorText)
{
    if (!isInit())
    {
        const std::string error = "pdf engine is not initialized";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::string error;
    if (filePath.empty() || !CSystem::fileExists(filePath))
    {
        error = filePath.empty() ? "file path is empty" : "file does not exist";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::unique_ptr<PdfDocument> document(new PdfDocument());
    if (!document->open(filePath, password, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    close();
    PdfDocument* documentPtr = document.get();
    m_documents.push_back(std::move(document));
    m_mainPath = filePath;
    m_mainPassword = password;
    for (int32_t i = 0; i < documentPtr->pageCount(); ++i)
    {
        PdfReaderCorePageEntry entry;
        entry.document = documentPtr;
        entry.pageIndex = i;
        entry.sourcePath = filePath;
        m_pages.push_back(entry);
    }
    m_lastError.clear();
    return true;
}

void PdfReaderCore::close()
{
    m_pages.clear();
    m_documents.clear();
    m_mainPath.clear();
    m_mainPassword.clear();
    m_lastError.clear();
}

bool PdfReaderCore::isOpen() const
{
    return !m_pages.empty();
}

std::wstring PdfReaderCore::filePath() const
{
    return m_mainPath;
}

std::string PdfReaderCore::lastError() const
{
    return m_lastError;
}

int32_t PdfReaderCore::pageCount() const
{
    return static_cast<int32_t>(m_pages.size());
}

bool PdfReaderCore::validatePageIndex(int32_t pageIndex, std::string* errorText)
{
    if (!isOpen())
    {
        if (errorText) *errorText = "no PDF document is open";
        return false;
    }
    if (pageIndex < 0 || pageIndex >= pageCount())
    {
        if (errorText) *errorText = "page index is out of range";
        return false;
    }
    return true;
}

bool PdfReaderCore::pageInfo(int32_t pageIndex,
                             PdfReaderCorePageInfo* info,
                             std::string* errorText)
{
    if (!info)
    {
        const std::string error = "page info output is null";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::string error;
    if (!validatePageIndex(pageIndex, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    PdfEnginePageSize size;
    const PdfReaderCorePageEntry& entry = m_pages[static_cast<size_t>(pageIndex)];
    if (!entry.document->getPageSize(entry.pageIndex, &size, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    info->width = size.width;
    info->height = size.height;
    m_lastError.clear();
    return true;
}

bool PdfReaderCore::renderPage(int32_t pageIndex,
                               int32_t pixelWidth,
                               int32_t pixelHeight,
                               PdfEngineBitmap* bitmap,
                               std::string* errorText)
{
    if (!bitmap || pixelWidth <= 0 || pixelHeight <= 0 ||
        static_cast<uint64_t>(pixelWidth) * static_cast<uint64_t>(pixelHeight) > m_config.maxRenderPixels)
    {
        const std::string error = "invalid render parameters";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::string error;
    if (!validatePageIndex(pageIndex, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    const PdfReaderCorePageEntry& entry = m_pages[static_cast<size_t>(pageIndex)];
    if (!entry.document->renderPageToBgra(entry.pageIndex, pixelWidth, pixelHeight,
                                          bitmap, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    m_lastError.clear();
    return true;
}

bool PdfReaderCore::insertDocument(const std::wstring& filePath,
                                   const std::string& password,
                                   int32_t insertIndex,
                                   std::string* errorText)
{
    if (!isOpen())
    {
        const std::string error = "no PDF document is open";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    if (insertIndex < 0 || insertIndex > pageCount())
    {
        const std::string error = "insert index is out of range";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::string error;
    PdfDocument* document = openDocument(filePath, password, &error);
    if (!document)
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::vector<PdfReaderCorePageEntry> inserted;
    for (int32_t i = 0; i < document->pageCount(); ++i)
    {
        PdfReaderCorePageEntry entry;
        entry.document = document;
        entry.pageIndex = i;
        entry.sourcePath = filePath;
        inserted.push_back(entry);
    }
    m_pages.insert(m_pages.begin() + insertIndex, inserted.begin(), inserted.end());
    m_lastError.clear();
    return true;
}

bool PdfReaderCore::movePage(int32_t fromIndex, int32_t toIndex, std::string* errorText)
{
    std::string error;
    if (!validatePageIndex(fromIndex, &error) ||
        toIndex < 0 || toIndex >= pageCount())
    {
        if (error.empty()) error = "destination page index is out of range";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    if (fromIndex != toIndex)
    {
        PdfReaderCorePageEntry entry = m_pages[static_cast<size_t>(fromIndex)];
        m_pages.erase(m_pages.begin() + fromIndex);
        m_pages.insert(m_pages.begin() + toIndex, entry);
    }
    m_lastError.clear();
    return true;
}

std::vector<PdfEnginePageRef> PdfReaderCore::pageRefs() const
{
    std::vector<PdfEnginePageRef> refs;
    refs.reserve(m_pages.size());
    for (size_t i = 0; i < m_pages.size(); ++i)
        refs.push_back(PdfEnginePageRef(m_pages[i].document, m_pages[i].pageIndex));
    return refs;
}

bool PdfReaderCore::saveTo(const std::wstring& outputFilePath,
                           std::string* errorText)
{
    if (!isOpen() || outputFilePath.empty())
    {
        const std::string error = outputFilePath.empty() ? "output path is empty" : "no PDF document is open";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::string error;
    if (!savePages(outputFilePath, pageRefs(), &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    m_lastError.clear();
    return true;
}

bool PdfReaderCore::saveToMain(std::string* errorText)
{
    if (m_mainPath.empty())
    {
        const std::string error = "main PDF path is empty";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    const std::string utf8Path = CStringManager::UnicodeToUtf8(m_mainPath);
    const std::wstring path = m_mainPath;
    const std::string password = m_mainPassword;
    const std::string temporaryUtf8 = utf8Path + ".pdfreader-" + CSystem::uuid() + ".tmp";
    const std::wstring temporaryPath = CStringManager::Utf8ToUnicode(temporaryUtf8);
    std::string error;
    if (!saveTo(temporaryPath, &error))
    {
        CSystem::deleteFile(temporaryPath);
        if (errorText) *errorText = error;
        return false;
    }
    close();
    if (!CSystem::replaceFile(temporaryPath, path))
    {
        error = "failed to replace the main PDF file; recovered PDF: " + temporaryUtf8;
        std::string restoreError;
        open(path, password, &restoreError);
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    if (!open(path, password, &error))
    {
        if (errorText) *errorText = error;
        return false;
    }
    return true;
}

bool PdfReaderCore::validatePageRange(const std::string& rangeText, std::string* errorText)
{
    std::vector<int32_t> indexes;
    return PdfEngine::parsePageRanges(rangeText, pageCount(), &indexes, errorText);
}

bool PdfReaderCore::savePageRange(const std::string& rangeText,
                                  const std::wstring& outputFilePath,
                                  std::string* errorText)
{
    if (!isOpen())
    {
        if (errorText) *errorText = "no PDF document is open";
        return false;
    }
    std::vector<int32_t> indexes;
    std::string error;
    if (!PdfEngine::parsePageRanges(rangeText, pageCount(), &indexes, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::vector<PdfEnginePageRef> refs;
    refs.reserve(indexes.size());
    for (size_t i = 0; i < indexes.size(); ++i)
    {
        const PdfReaderCorePageEntry& entry = m_pages[static_cast<size_t>(indexes[i])];
        refs.push_back(PdfEnginePageRef(entry.document, entry.pageIndex));
    }
    if (!savePages(outputFilePath, refs, &error))
    {
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    m_lastError.clear();
    return true;
}

bool PdfReaderCore::saveEachPage(const std::wstring& outputDirectory,
                                 const std::string& namePrefix,
                                 std::string* errorText, bool overwrite)
{
    if (!isOpen() || outputDirectory.empty())
    {
        const std::string error = outputDirectory.empty() ? "output directory is empty" : "no PDF document is open";
        setError(error);
        if (errorText) *errorText = error;
        return false;
    }
    std::string directory = CStringManager::UnicodeToUtf8(outputDirectory);
    if (!directory.empty() && directory[directory.size() - 1] != '/' && directory[directory.size() - 1] != '\\')
        directory += "/";
    const std::string prefix = namePrefix.empty() ? "page" : namePrefix;
    if (!overwrite)
    {
        // 写入任何页面之前检查全部目标，取消覆盖不能留下部分导出。
        for (int32_t i = 0; i < pageCount(); ++i)
        {
            const std::string output = PdfReaderCoreExportHelper::pageFileName(directory, prefix, i + 1, m_config.exportNumberWidth);
            if (CSystem::fileExists(CStringManager::Utf8ToUnicode(output)))
            {
                const std::string error = "output file already exists: " + output;
                setError(error);
                if (errorText) { *errorText = error; }
                return false;
            }
        }
    }
    for (int32_t i = 0; i < pageCount(); ++i)
    {
        const std::string output = PdfReaderCoreExportHelper::pageFileName(directory, prefix, i + 1, m_config.exportNumberWidth);
        const std::wstring outputPath = CStringManager::Utf8ToUnicode(output);
        std::string error;
        std::vector<PdfEnginePageRef> refs;
        refs.push_back(PdfEnginePageRef(m_pages[static_cast<size_t>(i)].document,
                                        m_pages[static_cast<size_t>(i)].pageIndex));
        if (!savePages(outputPath, refs, &error))
        {
            setError(error);
            if (errorText) *errorText = error;
            return false;
        }
    }
    m_lastError.clear();
    return true;
}

bool PdfReaderCore::savePages(const std::wstring& outputPath,
    const std::vector<PdfEnginePageRef>& refs, std::string* errorText)
{
    return PdfReaderCoreExportHelper::savePages(m_engine, outputPath, refs, errorText);
}

void PdfReaderCore::setError(const std::string& errorText)
{
    m_lastError = errorText;
}