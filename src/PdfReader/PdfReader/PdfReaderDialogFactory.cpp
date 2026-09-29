#include "PdfReaderDialogFactory.h"
#include "PdfReaderDialog.h"
#include "PdfReaderDialogView.h"
#include <memory>

CustomDialog* PdfReaderDialogFactory::createDialog(const DialogParam& param)
{
    if (dynamic_cast<const PdfReaderDialogParam*>(&param) == nullptr)
    {
        return nullptr;
    }
    std::unique_ptr<PdfReaderDialog> dialog(new PdfReaderDialog);
    std::unique_ptr<PdfReaderDialogView> view(new PdfReaderDialogView);
    // 先交给Qt父子所有权；setView后续抛出时外壳仍持有有效内容对象。
    view->setParent(dialog.get());
    PdfReaderDialogView* attachedView = view.release();
    if (!dialog->setView(attachedView))
    {
        return nullptr;
    }
    dialog->setShowMode(POP_DIALOG_SHOW_MODE);
    return dialog.release();
}