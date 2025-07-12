#include "ConfirmAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Config.h"
#include "CMouse/CMouseAPI.h"

bool ConfirmAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CMouse::MoveClick(m_vecConfirm[g_config.m_accountCount == 3 ? (m_accountIndex + 1) : 0], 0);
	return false;
}

void ConfirmAtomicTask::setConfirmParam(const std::vector<xyls::Point>& vecConfirm)
{
	m_vecConfirm = vecConfirm;
}