#include "AskDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "DialogManager.h"

int32_t AskDialog::popAskDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& acceptText,
								int32_t acceptDone,
								const QString& ignoreText,
								int32_t ignoreDone,
								QWindow* parent,
								int32_t timeOut,
								bool isCountDownVisible)
{
	AskDialog* dlg = new AskDialog(title, tip, acceptText, acceptDone, ignoreText, ignoreDone);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(ASK_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

AskDialog::AskDialog(const QString& title,
					 const QString& tip,
					 const QString& acceptText,
					 int32_t acceptDone,
					 const QString& ignoreText,
					 int32_t ignoreDone)
{
	initForExec(340, 165);
	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);
	m_tip = addTip(tip, QRect(43, 40, width() - 43 * 2, 83), QColor(205, 213, 225, 255));
	m_tip->setFontSize(12);
	m_accept = addButton(acceptText, QRect((width() - 116 * 2) / 3, 127, 116, 22), acceptDone);
	m_ignore = addButton(ignoreText, QRect((width() - 116 * 2) / 3 * 2 + 116, 127, 116, 22), ignoreDone);

	m_accept->installEventFilter(this);
	m_ignore->installEventFilter(this);

	QObject::connect(this, &DialogShow::keyboardAccept, this, &AskDialog::askAccept);
}

void AskDialog::askAccept(QObject* tar, Qt::Key key)
{
	m_accept->setFocus();
	m_accept->click();
}
