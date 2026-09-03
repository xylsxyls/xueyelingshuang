#include "PdfEngineInstance.h"

#include <pdfium/fpdfview.h>

#ifdef _MSC_VER
#pragma comment(lib, "pdfium.lib")
#endif

PdfEngineInstance& PdfEngineInstance::instance()
{
    static PdfEngineInstance s_instance;
    return s_instance;
}

PdfEngineInstance::PdfEngineInstance() :
m_isPdfiumInitialized(false),
m_initRefCount(0)
{

}

void PdfEngineInstance::addInitRef()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (!m_isPdfiumInitialized)
    {
        FPDF_InitLibrary();
        m_isPdfiumInitialized = true;
    }

    ++m_initRefCount;
}

void PdfEngineInstance::releaseInitRef()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    if (m_initRefCount > 0)
    {
        --m_initRefCount;
    }

    if (m_isPdfiumInitialized && m_initRefCount == 0)
    {
        FPDF_DestroyLibrary();
        m_isPdfiumInitialized = false;
    }
}

bool PdfEngineInstance::isPdfiumInitialized() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_isPdfiumInitialized;
}