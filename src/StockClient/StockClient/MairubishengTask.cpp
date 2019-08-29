#include "MairubishengTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "StockClient.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockClientLogicManager.h"

MairubishengTask::MairubishengTask():
m_sleepTime(0),
m_stockClient(nullptr),
m_exit(false)
{

}

void MairubishengTask::DoTask()
{
	CMouse::MoveAbsolute(xyls::Point(9, 252), 0);
	CMouse::LeftClick();
	Sleep(2000);
	CMouse::MoveAbsolute(xyls::Point(1379, 805), 0);
	CMouse::LeftManyClick(30, 10);
	CMouse::MoveAbsolute(xyls::Point(505, 803), 0);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1378, 122), 0);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1819, 10), 0);

	std::vector<std::string> vecValidStock;
	int32_t index = -1;
	while (index++ != m_stockClient->m_todayMarket.size() - 1)
	{
		if (m_exit)
		{
			return;
		}
		bool isST = atoi(m_stockClient->m_todayMarket[index][2].c_str());
		if (isST)
		{
			continue;
		}
		BigNumber price = m_stockClient->m_todayMarket[index][3].c_str();
		if (price < 10)
		{
			continue;
		}

		const std::string& stock = m_stockClient->m_todayMarket[index][0];
		
		CMouse::LeftClick();
		Sleep(10);
		CKeyboard::InputString(stock + "\n", 100);
		Sleep(m_sleepTime);
		if (ScreenScript::FindPic(CSystem::GetCurrentExePath() + "mai.png", xyls::Rect(1320, 630, 1370, 769)))
		{
			vecValidStock.push_back(stock);
			LOG_SEND("thread mairubisheng %s", stock.c_str());
			RCSend("mairubisheng = %s", stock.c_str());
		}
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("mairubisheng查看完成"));
}

void MairubishengTask::setParam(int32_t sleepTime, StockClient* stockClient)
{
	m_sleepTime = sleepTime;
	m_stockClient = stockClient;
}

void MairubishengTask::StopTask()
{
	m_exit = true;
}