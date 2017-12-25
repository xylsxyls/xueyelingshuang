#include "InputDialog.h"
#include "Label.h"
#include "LineEdit.h"

int32_t InputDialog::popInputDialog(const QString& title,
									const QString& editTip,
									const QString& buttonText,
									int32_t done,
									QString& editText,
									QDialog* parent,
									int32_t timeOut)
{
	InputDialog dlg(title, editTip, buttonText, done, editText);
	dlg.setResponseHighlightDialog(parent);
	return dlg.exec(timeOut);
}

InputDialog::InputDialog(const QString& title,
						 const QString& editTip,
						 const QString& buttonText,
						 int32_t done,
						 QString& editText):
m_editText(editText)
{
	m_title->setText(title);
	m_editTip = addLabel(editTip, QRect(43, 26, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
	m_edit = addLineEdit(QRect(52, 74, 234, 26), editText);
}

void InputDialog::reject()
{
	m_editText = m_edit->text();
	QDialog::reject();
}