#include "SplitViewerTestRuntime.h"
#include "SplitViewerTestConfig.h"
#include "../../SplitViewer/SplitViewer/SplitViewerTaskLifecycleManager.h"
#include "../../SplitViewer/SplitViewer/SplitViewerDialogHelper.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"

SplitViewerTestRuntime::SplitViewerTestRuntime()
{

}

bool SplitViewerTestRuntime::init()
{
    LogManager::instance().set(g_testConfig.m_logEnabled, g_testConfig.m_logConsoleEnabled);
    LogManager::instance().init();
    DialogManager::setLogCallback(SplitViewerDialogHelper::forwardLog);
    return SplitViewerTaskLifecycleManager::instance().init();
}

SplitViewerTestRuntime::~SplitViewerTestRuntime()
{
    SplitViewerTaskLifecycleManager::instance().finish();
    DialogManager::instance().uninit();
    DialogManager::releaseInstance();
    DialogManager::setLogCallback(nullptr);
    LogManager::instance().uninitAll();
}