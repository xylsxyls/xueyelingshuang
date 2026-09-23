#include "PdfReaderDialogFactory.h"
#include "PdfReaderDialogView.h"

CustomViewBase* PdfReaderDialogFactory::createView(const DialogParam& param)
{
    return dynamic_cast<const PdfReaderDialogParam*>(&param) ? new PdfReaderDialogView : nullptr;
}
