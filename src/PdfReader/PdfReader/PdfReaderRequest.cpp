#include "PdfReaderRequest.h"

PdfReaderRequest::PdfReaderRequest() : m_id(0), m_epoch(0), m_operation(PdfReaderRender),
m_index(0), m_target(0), m_overwrite(false)
{

}