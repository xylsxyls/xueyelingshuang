#include "PdfEngineFileWriter.h"

#include <new>

#include <pdfium/fpdf_save.h>

/** PDFium写文件结构和所属C++回调对象的绑定状态
*/
struct PdfEngineFileWriteState : public FPDF_FILEWRITE
{
public:
    // 所属C++写文件回调对象
    PdfEngineFileWriter* owner;

public:
    /** 构造空写文件绑定状态
    */
    PdfEngineFileWriteState();
};

PdfEngineFileWriteState::PdfEngineFileWriteState() :
owner(nullptr)
{
    version = 1;
    WriteBlock = nullptr;
}

PdfEngineFileWriter::PdfEngineFileWriter(FILE* file) :
m_file(file),
m_failed(false),
m_nativeWriter(nullptr)
{
    PdfEngineFileWriteState* state = new (std::nothrow) PdfEngineFileWriteState();
    if (state == nullptr)
    {
        return;
    }

    state->owner = this;
    state->WriteBlock = [](FPDF_FILEWRITE* self, const void* data, unsigned long size) -> int
    {
        PdfEngineFileWriteState* state = reinterpret_cast<PdfEngineFileWriteState*>(self);
        if (state == nullptr || state->owner == nullptr)
        {
            return 0;
        }

        return (*(state->owner))(data, size);
    };
    m_nativeWriter = state;
}

PdfEngineFileWriter::~PdfEngineFileWriter()
{
    delete reinterpret_cast<PdfEngineFileWriteState*>(m_nativeWriter);
    m_nativeWriter = nullptr;
}

FPDF_FILEWRITE* PdfEngineFileWriter::nativeWriter() const
{
    return m_nativeWriter;
}

bool PdfEngineFileWriter::failed() const
{
    return m_failed;
}

int PdfEngineFileWriter::operator()(const void* data, unsigned long size)
{
    if (m_file == nullptr || data == nullptr)
    {
        m_failed = true;
        return 0;
    }

    size_t written = fwrite(data, 1, size, m_file);
    if (written != size)
    {
        m_failed = true;
        return 0;
    }

    return 1;
}