#include "TipDialog.h"
#include "Label.h"

int32_t TipDialog::popTipDialog(int32_t& dialogId,
								const QString& title,
								const QString& tip,
								const QString& buttonText,
								int32_t done,
								QWindow* parent,
								int32_t timeOut,
								bool isCountDownVisible)
{
	TipDialog dlg(title, tip, buttonText, done);
	dlg.setParentWindow(parent);
	return dlg.exec(dialogId, timeOut, isCountDownVisible);
}

TipDialog::TipDialog(const QString& title,
					 const QString& tip,
					 const QString& buttonText,
					 int32_t done)
{
	initForExec();
	m_title->setText(title);
	m_tip = addTip(tip, QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
}