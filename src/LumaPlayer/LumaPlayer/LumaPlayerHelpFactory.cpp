#include "LumaPlayerHelpFactory.h"
#include "LumaPlayerHelpView.h"

CustomViewBase* LumaPlayerHelpFactory::createView(const DialogParam& param)
{
    const LumaPlayerHelpParam* help = dynamic_cast<const LumaPlayerHelpParam*>(&param);
    return help != nullptr ? new LumaPlayerHelpView(help->m_logic) : nullptr;
}

void LumaPlayerHelpFactory::destroy(CustomViewFactory* factory)
{
    delete factory;
}