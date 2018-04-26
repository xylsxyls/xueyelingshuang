#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

/** Ñ¯ÎÊµ¯³ö¿ò
*/
class AskDialog : public PopDialog
{
	Q_OBJECT
public:
    AskDialog();

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