#include "TieQiangTask.h"
#include "CMouse/CMouseAPI.h"
#include "DialogManager/DialogManagerAPI.h"

void TieQiangTask::DoTask()
{
	RCSend("step = %d", m_step);
	CMouse::MoveClick(xyls::Point(1741, 286), 500);
	CMouse::MoveClick(xyls::Point(886, 694), 1000);
	CMouse::MoveClick(xyls::Point(1321, 517), 10000);
	CMouse::MoveClick(xyls::Point(962, 601), 500);
	CMouse::MoveClick(xyls::Point(947, 546), 10000);
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("退出副本后点确定");
	DialogManager::instance().makeDialog(tipDialogParam);
}