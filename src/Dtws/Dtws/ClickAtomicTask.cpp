#include "ClickAtomicTask.h"
#include "Config.h"
#include "CMouse/CMouseAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"

ClickAtomicTask::ClickAtomicTask():
m_isStartWithFindPic(false),
m_isEndWithFindPic(false),
m_isReDo(false),
m_accountIndex(0)
{

}

void ClickAtomicTask::DoTask()
{
	CMouse::MoveAbsolute(g_screen.m_clickTop[m_accountIndex], 0);
	CMouse::MiddleClick();
	Sleep(50);
	
	std::vector<std::pair<std::string, xyls::Rect>> vecPic;
	vecPic = findPic(m_isStartWithFindPic ? m_vecStartFindPic : m_vecStartNotFindPic);
	m_isReDo = DoFun(vecPic);
	if (m_isReDo)
	{
		m_end.signal();
		return;
	}
	vecPic = findPic(m_isEndWithFindPic ? m_vecEndFindPic : m_vecEndNotFindPic);
	m_isReDo = CheckFun(vecPic);

	m_end.signal();
}

void ClickAtomicTask::startWithFindPic(
	const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecStartFindPic)
{
	m_vecStartFindPic = vecStartFindPic;
	m_isStartWithFindPic = true;
}

void ClickAtomicTask::startWithNotFindPic(
	const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecStartNotFindPic)
{
	m_vecStartNotFindPic = vecStartNotFindPic;
	m_isStartWithFindPic = false;
}

bool ClickAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	return false;
}

bool ClickAtomicTask::CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	return false;
}

void ClickAtomicTask::endWithFindPic(
	const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecEndFindPic)
{
	m_vecEndFindPic = vecEndFindPic;
	m_isEndWithFindPic = true;
}

void ClickAtomicTask::endWithNotFindPic(
	const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecEndNotFindPic)
{
	m_vecEndNotFindPic = vecEndNotFindPic;
	m_isEndWithFindPic = false;
}

bool ClickAtomicTask::isReDo()
{
	return m_isReDo;
}

void ClickAtomicTask::setAccountIndex(int32_t accountIndex)
{
	m_accountIndex = accountIndex;
}

void ClickAtomicTask::waitForEnd()
{
	m_end.wait();
}

std::vector<std::pair<std::string, xyls::Rect>> ClickAtomicTask::findPic(
	const std::vector<std::pair<std::string, std::pair<xyls::Rect, double>>>& vecPic)
{
	std::vector<std::pair<std::string, xyls::Rect>> result;
	int32_t index = -1;
	while (index++ != vecPic.size() - 1)
	{
		xyls::Rect picRect = ScreenScript::FindPic(vecPic[index].first,
			vecPic[index].second.first, vecPic[index].second.second);
		if (!picRect.empty())
		{
			result.push_back({ vecPic[index].first, picRect });
			continue;
		}
		Sleep(g_config.m_secondFindTime);
		picRect = ScreenScript::FindPic(vecPic[index].first,
			vecPic[index].second.first, vecPic[index].second.second);
		if (!picRect.empty())
		{
			result.push_back({ vecPic[index].first, picRect });
			continue;
		}
	}
	return result;
}