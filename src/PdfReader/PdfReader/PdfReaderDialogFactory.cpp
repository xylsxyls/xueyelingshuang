#include "PdfReaderDialogFactory.h"
#include "PdfReaderDialog.h"
#include "PdfReaderDialogView.h"
#include <new>

CustomDialog* PdfReaderDialogFactory::createDialog(const DialogParam& param)
{
    if (dynamic_cast<const PdfReaderDialogParam*>(&param) == nullptr)
    {
        return nullptr;
    }
    PdfReaderDialog* dialog = new (std::nothrow) PdfReaderDialog;
    PdfReaderDialogView* view = new (std::nothrow) PdfReaderDialogView;
    if (dialog == nullptr || view == nullptr || !dialog->setView(view))
    {
        delete view;
        delete dialog;
        return nullptr;
    }
    dialog->setShowMode(POP_DIALOG_SHOW_MODE);
    return dialog;
}