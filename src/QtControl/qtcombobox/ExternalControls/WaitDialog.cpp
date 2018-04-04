#include "WaitDialog.h"
#include "Label.h"
#include "DialogManager.h"

int32_t WaitDialog::popWaitDialog(int32_t& dialogId,
								  int32_t taskId,
								  const QString& title,
								  const QString& tip,
								  QWindow* parent,
								  int32_t timeOut,
								  bool isCountDownVisible)
{
	WaitDialog* dlg = new WaitDialog(title, tip);
	dlg->setParentWindow(parent);
	dlg->setDialogEnum(WAIT_DIALOG);
	dlg->setTaskId(taskId);
	int32_t result = dlg->exec(dialogId, timeOut, isCountDownVisible);
	delete dlg;
	return result;
}

void WaitDialog::setTaskId(int32_t taskId)
{
	m_taskId = taskId;
}

int32_t WaitDialog::getTaskId()
{
	return m_taskId;
}

WaitDialog::WaitDialog(const QString& title, const QString& tip) :
m_taskId(0)
{
	initForExec(340, 165);
	m_title->setText(title);
	setWindowTitle(title);
	m_tip = addTip(tip, QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	setExitVisible(false);
}