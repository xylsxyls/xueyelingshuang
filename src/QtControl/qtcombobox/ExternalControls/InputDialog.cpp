#include "InputDialog.h"
#include "Label.h"
#include "LineEdit.h"
#include "../COriginalButton.h"
#include "DialogManager.h"

int32_t InputDialog::popInputDialog(int32_t& dialogId,
									const QString& title,
									const QString& editTip,
									const QString& buttonText,
									int32_t done,
									QString& editText,
									bool isPassword,
									int32_t maxLength,
									QWindow* parent,
									int32_t timeOut,
									bool isCountDownVisible)
{
	InputDialog* dlg = new InputDialog(title, editTip, buttonText, done, editText, isPassword, maxLength);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(INPUT_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

InputDialog::InputDialog(const QString& title,
						 const QString& editTip,
						 const QString& buttonText,
						 int32_t done,
						 QString& editText,
						 bool isPassword,
						 int32_t maxLength) :
m_editText(editText)
{
	initForExec(340, 165);
	setFocus();
	setWindowTitle(title);
	m_title->setText(title);
	m_title->setFontSize(12);
	m_editTip = addLabel(editTip, QRect(43, 26, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	m_editTip->setFontSize(12);
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
	m_edit = addLineEdit(QRect(52, 74, 234, 26), isPassword, editText);
	if (maxLength >= 0)
	{
		m_edit->setMaxLength(maxLength);
	}
	m_edit->setFocus();

	m_accept->installEventFilter(this);

	QObject::connect(this, SIGNAL(keyboardAccept()), this, SLOT(inputAccept()));
}

void InputDialog::done(int result)
{
	m_editText = m_edit->text();
	QDialog::done(result);
}

void InputDialog::inputAccept()
{
	m_accept->setFocus();
	m_accept->click();
}
