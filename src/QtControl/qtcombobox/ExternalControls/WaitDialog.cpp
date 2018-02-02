#include "WaitDialog.h"
#include "Label.h"

int32_t WaitDialog::popWaitDialog(int32_t& dialogId,
								  const QString& title,
								  const QString& tip,
								  QWindow* parent,
								  int32_t timeOut,
								  bool isCountDownVisible)
{
	WaitDialog dlg(title, tip);
	dlg.setParentWindow(parent);
	return dlg.exec(dialogId, timeOut, isCountDownVisible);
}

WaitDialog::WaitDialog(const QString& title, const QString& tip)
{
	initForExec();
	m_title->setText(title);
	setWindowTitle(title);
	m_tip = addTip(tip, QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	setExitVisible(false);
}