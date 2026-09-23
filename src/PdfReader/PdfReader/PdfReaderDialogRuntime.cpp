#include "PdfReaderDialogRuntime.h"
#include "PdfReaderDialogFactory.h"
#include "PdfReaderDialogParam.h"
#include <stdexcept>

PdfReaderDialogRuntime::PdfReaderDialogRuntime()
{
    PdfReaderDialogFactory* factory = new PdfReaderDialogFactory;
    if (!DialogManager::instance().registerCustomViewFactory(static_cast<DialogType>(PdfReaderDialogParam::TypeId), factory))
    {
        delete factory;
        throw std::runtime_error("PdfReader dialog factory registration failed");
    }
}

PdfReaderDialogRuntime::~PdfReaderDialogRuntime()
{
    DialogManager::instance().uninit();
    DialogManager::releaseInstance();
}
