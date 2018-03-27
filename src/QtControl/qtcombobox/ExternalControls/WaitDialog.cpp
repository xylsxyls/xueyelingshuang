#include "WaitDialog.h"
#include "Label.h"
#include "DialogManager.h"

int32_t WaitDialog::popWaitDialog(int32_t& dialogId,
								  const QString& title,
								  const QString& tip,
								  QWindow* parent,
								  int32_t timeOut,
								  bool isCountDownVisible)
{
	WaitDialog* dlg = new WaitDialog(title, tip);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(WAIT_DIALOG);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	dlg->deleteLater();
	return result;
}

WaitDialog::WaitDialog(const QString& title, const QString& tip)
{
	initForExec(340, 165);
	m_title->setText(title);
	setWindowTitle(title);
	m_tip = addTip(tip, QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	setExitVisible(false);
}