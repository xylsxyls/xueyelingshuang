#include "stdafx.h"
#include "Config.h"
#include "name.h"

CConfig::CConfig()
{
	m_screenWidth = GetSystemMetrics(SM_CXSCREEN);
	m_screenHeight = GetSystemMetrics(SM_CYSCREEN);

	m_screenType = (m_screenWidth == 3840 ? 0 : 1);

	//3840 * 2160
	if (m_screenType == 0)
	{
		m_gameScreenHeight = 1646;
		m_side = 150;
		m_showSide = 75;
		m_heroHeadSpace = 40;
		m_heroHeadShowPoint = { 817, 468 };
		m_editHeroPoint = { 2000, 1000 };
		m_equipBeginPoint = { 1384, 1699 };
		m_equipWidth = 238;
		m_wCenterPoint = { 3020, 1398 };
		m_rCenterPoint = { 3323, 1219 };
		m_fCenterPoint = { 2584, 1736 };
		m_bCenterPoint = { 2136, 1735 };
		m_heroCenterPoint = { 3840 / 2, 1236 };
		m_speakBeginPoint = { 3479, 910 };
		m_speakEndPoint = { 3046, 1535 };
		m_heroHeadPoint[0] = { 2427, 1386 };
		m_heroHeadPoint[1] = { 2488, 1203 };
		m_heroHeadPoint[2] = { 2598, 1040 };
		m_heroHeadPoint[3] = { 2757, 923 };
		m_heroHeadPoint[4] = { 2929, 842 };
	}
	//1920 * 1080
	else
	{
		m_gameScreenHeight = 823;
		m_side = 75;
		m_showSide = 40;
		m_heroHeadSpace = 20;
		m_heroHeadShowPoint = { 407, 232 };
		m_editHeroPoint = { 1000, 500 };
		m_equipBeginPoint = { 612, 952 };
		m_equipWidth = 160;
		m_wCenterPoint = { 1542, 749 };
		m_rCenterPoint = { 1663, 609 };
		m_fCenterPoint = { 1292, 869 };
		m_bCenterPoint = { 1061, 860 };
		m_heroCenterPoint = { 1920 / 2, 618 };
		m_speakBeginPoint = { 1740, 448 };
		m_speakEndPoint = { 1517, 764 };
		m_heroHeadPoint[0] = { 1215, 696 };
		m_heroHeadPoint[1] = { 1243, 603 };
		m_heroHeadPoint[2] = { 1298, 525 };
		m_heroHeadPoint[3] = { 1376, 465 };
		m_heroHeadPoint[4] = { 1465, 422 };
	}

	m_heroName.push_back(DF);
	m_heroName.push_back(JIE);
	m_heroName.push_back(YJ);
	m_heroName.push_back(KSD);
	m_heroName.push_back(KTLN);
	m_heroName.push_back(JYX);
	m_heroName.push_back(WZ);
	m_heroName.push_back(ZGL);
	m_heroName.push_back(ZK);
	m_heroName.push_back(SMY);
	m_heroName.push_back(SWK);
	m_heroName.push_back(LLW);
	m_heroName.push_back(BLSY);
	m_heroName.push_back(HX);
	m_heroName.push_back(NKLL);
	m_heroName.push_back(BLXC);
	m_heroName.push_back(ZY);
}

CConfig& CConfig::instance()
{
	static CConfig s_cconfig;
	return s_cconfig;
}

int32_t CConfig::nameType(const std::string& name)
{
	auto it = std::find(m_heroName.begin(), m_heroName.end(), name);
	if (it == m_heroName.end())
	{
		return -1;
	}
	return std::distance(m_heroName.begin(), it) + 1;
}

bool CConfig::timerMs(int32_t timeMs, int32_t msTimes)
{
	return msTimes % (timeMs / m_msTime) == 0;
}