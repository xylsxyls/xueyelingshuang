#pragma once

#include "DialogManager/DialogManagerAPI.h"

class QResizeEvent;

/** PdfReader custom dialog shell; About and export-range dialogs show a title-bar close button. */
class PdfReaderDialog : public CustomDialog
{
public:
    /** Create the PdfReader dialog shell. */
    PdfReaderDialog();
    /** Initialize the title, content area, and dialog-specific close button. */
    bool initDialog(const DialogParam& param) override;

protected:
    /** Keep the close button aligned when the title bar changes size. */
    void resizeEvent(QResizeEvent* event) override;

private:
    /** Configure and place the title-bar close button. */
    void updateTitleCloseButtonGeometry();
};