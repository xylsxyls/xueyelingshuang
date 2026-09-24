#pragma once
#include "DialogManager/DialogManagerAPI.h"

/** Register the PdfReader dialog shell; ownership is transferred to DialogManager. */
class PdfReaderDialogFactory : public CustomDialogFactory
{
public:
    /** Create a dialog shell and its content view.
    @param [in] param Dialog parameters.
    @return A dialog for a matching parameter type, otherwise nullptr.
    */
    CustomDialog* createDialog(const DialogParam& param) override;
};