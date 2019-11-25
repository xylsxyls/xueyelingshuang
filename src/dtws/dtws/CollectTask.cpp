#include "CollectTask.h"
#include "DialogManager/DialogManagerAPI.h"

void CollectTask::DoTask()
{
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("收集完成后点确定");
	DialogManager::instance().makeDialog(tipDialogParam);
}