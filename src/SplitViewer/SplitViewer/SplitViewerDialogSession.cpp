#include "SplitViewerDialogSession.h"
#include "SplitViewerFileDialogFactory.h"
#include "SplitViewerAboutDialogFactory.h"
#include "Config.h"
#include <memory>
#include <stdexcept>

SplitViewerDialogSession::SplitViewerDialogSession()
{
    std::unique_ptr<SplitViewerFileDialogFactory> factory(new SplitViewerFileDialogFactory);
    if (!DialogManager::instance().registerCustomViewFactory(g_config.m_fileDialogType,
        factory.get(), POP_DIALOG_SHOW_MODE, &SplitViewerFileDialogFactory::destroy))
    {
        throw std::runtime_error("SplitViewer file dialog factory registration failed");
    }
    factory.release();

    std::unique_ptr<SplitViewerAboutDialogFactory> aboutFactory(new SplitViewerAboutDialogFactory);
    if (!DialogManager::instance().registerCustomDialogFactory(g_config.m_aboutDialogType,
        aboutFactory.get(), &SplitViewerAboutDialogFactory::destroy))
    {
        throw std::runtime_error("SplitViewer about dialog factory registration failed");
    }
    aboutFactory.release();
}

SplitViewerDialogSession::~SplitViewerDialogSession()
{
    DialogManager::instance().uninit();
    DialogManager::releaseInstance();
}