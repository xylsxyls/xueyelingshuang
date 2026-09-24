#pragma once

#include "DialogManager/DialogManagerAPI.h"

class SplitViewerAboutDialogFactory : public CustomDialogFactory
{
public:
    CustomDialog* createDialog(const DialogParam& param) override;
    static void destroy(CustomDialogFactory* factory);
};