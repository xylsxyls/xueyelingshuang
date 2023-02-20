#include "stdafx.h"
#include "Config.h"
#include "name.h"

CConfig::CConfig()
{
	m_heroHeadPoint[0] = { 1215, 696 };
	m_heroHeadPoint[1] = { 1243, 603 };
	m_heroHeadPoint[2] = { 1298, 525 };
	m_heroHeadPoint[3] = { 1376, 465 };
	m_heroHeadPoint[4] = { 1465, 422 };

	m_equipPoint[0] = { 612, 952 };
	m_equipPoint[1] = { 772, 948 };
	m_equipPoint[2] = { 936, 951 };
	m_equipPoint[3] = { 1090, 947 };
	m_equipPoint[4] = { 1266, 941 };
	m_equipPoint[5] = { 1419, 949 };

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