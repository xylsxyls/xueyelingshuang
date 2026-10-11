#include "LumaPlayerDialogSession.h"
#include "LumaPlayerHelpFactory.h"
#include "LumaPlayerKaraokeProgressView.h"
#include "LumaPlayerRecordingSaveView.h"
#include "LumaPlayerMicrophoneView.h"
#include "DialogManager/DialogManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <memory>

LumaPlayerDialogSession::LumaPlayerDialogSession()
{
    std::unique_ptr<LumaPlayerMicrophoneFactory> microphone(new LumaPlayerMicrophoneFactory());
    if (DialogManager::instance().registerCustomViewFactory(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 3),
            microphone.get(), MODELESS_DIALOG_SHOW_MODE, &LumaPlayerMicrophoneFactory::destroy))
    {
        microphone.release();
    }
    else
    {
        LOGERROR("Microphone panel factory registration failed");
    }
    std::unique_ptr<LumaPlayerRecordingSaveFactory> save(new LumaPlayerRecordingSaveFactory());
    if (DialogManager::instance().registerCustomViewFactory(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 2),
            save.get(), POP_DIALOG_SHOW_MODE, &LumaPlayerRecordingSaveFactory::destroy))
    {
        save.release();
    }
    else
    {
        LOGERROR("Recording save factory registration failed");
    }
    std::unique_ptr<LumaPlayerKaraokeProgressFactory> separation(new LumaPlayerKaraokeProgressFactory());
    if (DialogManager::instance().registerCustomViewFactory(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 1),
            separation.get(), POP_DIALOG_SHOW_MODE, &LumaPlayerKaraokeProgressFactory::destroy))
    {
        separation.release();
    }
    else
    {
        LOGERROR("Separation progress factory registration failed");
    }
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