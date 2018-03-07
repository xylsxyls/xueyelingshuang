#include "TipDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "DialogManager.h"

int32_t TipDialog::popTipDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& buttonText,
								int32_t done,
								QWindow* parent,
								int32_t timeOut,
								bool isCountDownVisible)
{
	TipDialog* dlg = new TipDialog(title, tip, buttonText, done);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(TIP_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

TipDialog::TipDialog(const QString& title,
					 const QString& tip,
					 const QString& buttonText,
					 int32_t done)
{
	initForExec(340, 165);
	m_title->setText(title);
	setWindowTitle(title);
	m_title->setFontSize(12);
	m_tip = addTip(tip, QRect(43, 40, width() - 43 * 2, 83), QColor(205, 213, 225, 255));
	m_tip->setFontSize(12);
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
	m_accept->installEventFilter(this);

	QObject::connect(this, &DialogShow::keyboardAccept, this, &TipDialog::tipAccept);
}

void TipDialog::tipAccept()
{
	m_accept->setFocus();
	m_accept->click();
}
