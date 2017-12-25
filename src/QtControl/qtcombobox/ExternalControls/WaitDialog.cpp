#include "WaitDialog.h"
#include "Label.h"

int32_t WaitDialog::popWaitDialog(const QString& title,
							 const QString& tip,
							 QDialog* parent,
							 int32_t timeOut)
{
	WaitDialog dlg(title, tip);
	dlg.setResponseHighlightDialog(parent);
	return dlg.exec(timeOut);
}

WaitDialog::WaitDialog(const QString& title, const QString& tip)
{
	m_title->setText(title);
	m_tip = addTip(tip, QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	setExitVisible(false);
}