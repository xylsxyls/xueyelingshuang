#include "CollectTask.h"
#include "DialogManager/DialogManagerAPI.h"

void CollectTask::DoTask()
{
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("�ռ���ɺ��ȷ��");
	DialogManager::instance().makeDialog(tipDialogParam);
}