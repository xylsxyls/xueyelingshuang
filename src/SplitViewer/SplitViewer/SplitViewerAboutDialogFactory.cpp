#include "SplitViewerAboutDialogFactory.h"

#include "SplitViewerAboutDialog.h"
#include "SplitViewerAboutDialogParam.h"
#include "SplitViewerAboutDialogView.h"
#include <memory>

CustomDialog* SplitViewerAboutDialogFactory::createDialog(const DialogParam& param)
{
    if (dynamic_cast<const SplitViewerAboutDialogParam*>(&param) == nullptr)
    {
        return nullptr;
    }
    std::unique_ptr<SplitViewerAboutDialog> dialog(new SplitViewerAboutDialog);
    std::unique_ptr<SplitViewerAboutDialogView> view(new SplitViewerAboutDialogView);
    if (!dialog->setView(view.get()))
    {
        return nullptr;
    }
    view.release();
    dialog->setShowMode(POP_DIALOG_SHOW_MODE);
    return dialog.release();
}

void SplitViewerAboutDialogFactory::destroy(CustomDialogFactory* factory)
{
    delete factory;
}