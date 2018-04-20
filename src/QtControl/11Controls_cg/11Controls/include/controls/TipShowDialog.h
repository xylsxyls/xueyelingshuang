#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "NotifyDialog.h"

/** œ‘ æÃ· æøÚ
*/
class TipShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
	TipShowDialog();

public:
    void setTip(const QString& tip);

    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

protected:
    void resizeEvent(QResizeEvent* eve);

private:
	Label* m_tip;
	COriginalButton* m_accept;
};