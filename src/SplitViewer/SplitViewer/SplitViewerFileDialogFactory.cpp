#include "SplitViewerFileDialogFactory.h"
#include "SplitViewerFileDialogView.h"

CustomViewBase* SplitViewerFileDialogFactory::createView(const DialogParam& param)
{
    return dynamic_cast<const SplitViewerFileDialogParam*>(&param) != nullptr ? new SplitViewerFileDialogView : nullptr;
}

void SplitViewerFileDialogFactory::destroy(CustomViewFactory* factory)
{
    delete factory;
}