#include "SplitViewerDialogSession.h"
#include "SplitViewerFileDialogFactory.h"
#include "SplitViewerAboutDialogFactory.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include <memory>
#include <new>

SplitViewerDialogSession::SplitViewerDialogSession() :
m_initialized(false)
{

}

SplitViewerDialogSession::~SplitViewerDialogSession()
{
    uninit();
}

bool SplitViewerDialogSession::init()
{
    if (m_initialized)
    {
        return true;
    }
    std::unique_ptr<SplitViewerFileDialogFactory> factory(new (std::nothrow) SplitViewerFileDialogFactory);
    std::unique_ptr<SplitViewerAboutDialogFactory> aboutFactory(new (std::nothrow) SplitViewerAboutDialogFactory);
    if (!factory || !aboutFactory)
    {
        LOGERROR("SplitViewer dialog factory allocation failed");
        return false;
    }
    if (!DialogManager::instance().registerCustomViewFactory(
            g_config.m_fileDialogType, factory.get(), POP_DIALOG_SHOW_MODE, &SplitViewerFileDialogFactory::destroy))
    {
        LOGERROR("SplitViewer file dialog factory registration failed; type=%d", g_config.m_fileDialogType);
        return false;
    }
    factory.release();

    if (!DialogManager::instance().registerCustomDialogFactory(
            g_config.m_aboutDialogType, aboutFactory.get(), &SplitViewerAboutDialogFactory::destroy))
    {
        DialogManager::instance().unregisterCustomViewFactory(g_config.m_fileDialogType);
        LOGERROR("SplitViewer about dialog factory registration failed; type=%d", g_config.m_aboutDialogType);
        return false;
    }
    aboutFactory.release();
    m_initialized = true;
    return true;
}

void SplitViewerDialogSession::uninit()
{
    if (!m_initialized)
    {
        return;
    }
    m_initialized = false;
    DestroyAllOperateParam close;
    DialogManager::instance().operateDialog(close);
    DialogManager::instance().unregisterCustomDialogFactory(g_config.m_aboutDialogType);
    DialogManager::instance().unregisterCustomViewFactory(g_config.m_fileDialogType);
}