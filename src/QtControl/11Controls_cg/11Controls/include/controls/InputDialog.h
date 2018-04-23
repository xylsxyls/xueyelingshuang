#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

class LineEdit;
/** ÊäÈëµ¯³ö¿ò
*/
class InputDialog : public PopDialog
{
	Q_OBJECT
public:
	InputDialog();

public:
    void setLineEdit(const QString& defaultText, QString* editText, int32_t maxLength = -1);

    void setTip(const QString& tip);

    void setAcceptButton(const QString& acceptText, int32_t acceptDone);

	void done(int result);

protected:
    void resizeEvent(QResizeEvent* eve);

private:
	Label* m_editTip;
	COriginalButton* m_accept;
	LineEdit* m_edit;
	QString* m_editText;
};