#include "SplitViewerDialogSession.h"
#include "SplitViewerFileDialogFactory.h"
#include "Config.h"
#include <memory>
#include <stdexcept>

SplitViewerDialogSession::SplitViewerDialogSession()
{
    std::unique_ptr<SplitViewerFileDialogFactory> factory(new SplitViewerFileDialogFactory);
    if (!DialogManager::instance().registerCustomViewFactory(Config::kFileDialogType,
        factory.get(), POP_DIALOG_SHOW_MODE, &SplitViewerFileDialogFactory::destroy))
    {
        throw std::runtime_error("SplitViewer file dialog factory registration failed");
    }
    factory.release();
}

SplitViewerDialogSession::~SplitViewerDialogSession()
{
    DialogManager::instance().uninit();
    DialogManager::releaseInstance();
}