#include "LumaPlayerDialogSession.h"
#include "LumaPlayerHelpFactory.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <memory>

LumaPlayerDialogSession::LumaPlayerDialogSession()
{
    std::unique_ptr<LumaPlayerHelpFactory> factory(new LumaPlayerHelpFactory());
    if (DialogManager::instance().registerCustomViewFactory(CUSTOM_DIALOG_TYPE_BEGIN,
        factory.get(), POP_DIALOG_SHOW_MODE, &LumaPlayerHelpFactory::destroy))
    {
        factory.release();
    }
    else
    {
        LOGERROR("Help factory registration failed");
    }
}

LumaPlayerDialogSession::~LumaPlayerDialogSession()
{
    DialogManager::instance().uninit();
    DialogManager::releaseInstance();
}