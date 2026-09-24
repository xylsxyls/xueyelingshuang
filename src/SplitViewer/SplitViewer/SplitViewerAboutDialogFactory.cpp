#include "SplitViewerAboutDialogFactory.h"

#include "SplitViewerAboutDialog.h"
#include "SplitViewerAboutDialogParam.h"
#include "SplitViewerAboutDialogView.h"
#include <new>

CustomDialog* SplitViewerAboutDialogFactory::createDialog(const DialogParam& param)
{
    if (dynamic_cast<const SplitViewerAboutDialogParam*>(&param) == nullptr)
    {
        return nullptr;
    }
    SplitViewerAboutDialog* dialog = new (std::nothrow) SplitViewerAboutDialog;
    SplitViewerAboutDialogView* view = new (std::nothrow) SplitViewerAboutDialogView;
    if (dialog == nullptr || view == nullptr || !dialog->setView(view))
    {
        delete view;
        delete dialog;
        return nullptr;
    }
    dialog->setShowMode(POP_DIALOG_SHOW_MODE);
    return dialog;
}

void SplitViewerAboutDialogFactory::destroy(CustomDialogFactory* factory)
{
    delete factory;
}