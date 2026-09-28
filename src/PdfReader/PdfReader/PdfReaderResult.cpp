#include "PdfReaderResult.h"

PdfReaderResult::PdfReaderResult() : m_success(false), m_cancelled(false),
m_code(PdfReaderCoreCResultInternalError), m_hasSnapshot(false)
{

}