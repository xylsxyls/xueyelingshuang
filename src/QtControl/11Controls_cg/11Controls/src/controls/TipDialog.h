#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

/** ÌáÊ¾µ¯³ö¿ò
*/
class TipDialog : public PopDialog
{
	Q_OBJECT
public:
	TipDialog();

public:
    void setTip(const QString& tip);

    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

protected:
    void resizeEvent(QResizeEvent* eve);

private:
	Label* m_tip;
	COriginalButton* m_accept;
};