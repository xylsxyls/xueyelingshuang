#include "MairubishengTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"

MairubishengTask::MairubishengTask():
m_vecValidStock(nullptr),
m_sleepTime(0)
{

}

void MairubishengTask::DoTask()
{
	CMouse::LeftClick(0);
	Sleep(10);
	CKeyboard::InputString(m_stock + "\n", 0);
	Sleep(m_sleepTime);
	bool isFind = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "mai.png", xyls::Rect(1320, 630, 1370, 769));//
	if (isFind)
	{
		m_vecValidStock->push_back(m_stock);
		LOG_SEND("thread %s", m_stock.c_str());
	}
}

void MairubishengTask::setParam(const std::string& stock, int32_t sleepTime, std::vector<std::string>* vecValidStock)
{
	m_stock = stock;
	m_sleepTime = sleepTime;
	m_vecValidStock = vecValidStock;
}