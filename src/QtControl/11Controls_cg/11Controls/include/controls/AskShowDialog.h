#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "NotifyDialog.h"

class COriginalButton;
/** œ‘ æÃ· æøÚ
*/
class AskShowDialog : public NotifyDialog
{
	Q_OBJECT
public:
	AskShowDialog();

public:
    void setTip(const QString& tip);

    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

    void setIgnoreButton(const QString& ignoreText, int32_t ignoreDone);

protected:
    void resizeEvent(QResizeEvent* eve);

private:
    bool check();

private:
	Label* m_tip;
	COriginalButton* m_accept;
	COriginalButton* m_ignore;
};