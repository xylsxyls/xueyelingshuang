#include "PdfEnginePageRef.h"

PdfEnginePageRef::PdfEnginePageRef() :
document(nullptr),
pageIndex(-1)
{

}

PdfEnginePageRef::PdfEnginePageRef(PdfDocument* doc, int32_t indexInDocument) :
document(doc),
pageIndex(indexInDocument)
{

}