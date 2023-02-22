#include "Config.h"
#include "CSystem/CSystemAPI.h"

CConfig::CConfig()
{
	m_accountPoint = { { 537, 1057 }, { 599, 1058 }, { 659, 1059 } };
	m_rightTopRect = { { 1534, 169, 1654, 262 }, { 1211, 477, 1340, 569 }, { 1854, 486, 1920, 561 } };
	m_clickTop = { { 455, 11 }, { 123, 321 }, { 1738, 322 } };
	m_talkPoint = { { 902, 139 }, { 590, 450 }, { 1223, 450 } };
	m_talkheadRect = { { 650, 9, 998, 496 }, { 326, 318, 694, 719 }, { 992, 325, 1333, 757 } };
	m_chatRect = { { 324, 465, 670, 677 }, { 0, 774, 350, 984 }, { 632, 772, 984, 988 } };
	m_bloodRect = { { 571, 61, 1156, 149 }, { 566, 377, 827, 470 }, { 1202, 384, 1464, 460 } };
	m_taskPoint = { { 1398, 288 }, { 1072, 595 }, { 1708, 597 } };
	m_intoGamePoint = { { 1024, 634 }, { 681, 943 }, { 1311, 940 } };

	m_currentExePath = CSystem::GetCurrentExePath();
}

CConfig& CConfig::instance()
{
	static CConfig s_cconfig;
	return s_cconfig;
}