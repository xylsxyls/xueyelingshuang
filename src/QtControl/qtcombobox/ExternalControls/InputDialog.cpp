#include "InputDialog.h"
#include "Label.h"
#include "LineEdit.h"

int32_t InputDialog::popInputDialog(int32_t& dialogId,
									const QString& title,
									const QString& editTip,
									const QString& buttonText,
									int32_t done,
									QString& editText,
									QWidget* parent,
									int32_t timeOut,
									bool isCountDownVisible)
{
	InputDialog dlg(title, editTip, buttonText, done, editText);
	dlg.setResponseHighlightDialog(parent);
	return dlg.exec(dialogId, timeOut, isCountDownVisible);
}

InputDialog::InputDialog(const QString& title,
						 const QString& editTip,
						 const QString& buttonText,
						 int32_t done,
						 QString& editText):
m_editText(editText)
{
	initForExec();
	m_title->setText(title);
	m_editTip = addLabel(editTip, QRect(43, 26, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
	m_edit = addLineEdit(QRect(52, 74, 234, 26), editText);
}

void InputDialog::done(int result)
{
	m_editText = m_edit->text();
	QDialog::done(result);
}