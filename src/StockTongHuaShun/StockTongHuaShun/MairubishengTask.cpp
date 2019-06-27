#include "MairubishengTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"

MairubishengTask::MairubishengTask():
m_vecValidStock(nullptr)
{

}

void MairubishengTask::DoTask()
{
	CKeyboard::InputString(m_stock + "\n");
	Sleep(200);
	bool isFind = ScreenScript::FindPic(CSystem::GetCurrentExePath() + "mai.png", xyls::Rect(1320, 630, 1370, 769));//
	if (isFind)
	{
		m_vecValidStock->push_back(m_stock);
		LOG_SEND("thread %s", m_stock.c_str());
	}
}

void MairubishengTask::setParam(const std::string& stock, std::vector<std::string>* vecValidStock)
{
	m_stock = stock;
	m_vecValidStock = vecValidStock;
}