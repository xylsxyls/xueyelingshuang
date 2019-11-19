#include "TuLangTask.h"
#include "CMouse/CMouseAPI.h"
#include "DialogManager/DialogManagerAPI.h"

void TuLangTask::DoTask()
{
	RCSend("step = %d", m_step);
	CMouse::MoveClick(xyls::Point(1741, 286), 500);
	CMouse::MoveClick(xyls::Point(874, 697), 35000);
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("退出副本后点确定");
	DialogManager::instance().makeDialog(tipDialogParam);
}