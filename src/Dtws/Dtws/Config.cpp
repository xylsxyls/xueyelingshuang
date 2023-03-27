#include "Config.h"
#include "CSystem/CSystemAPI.h"

CConfig::CConfig()
{
	m_accountConfig[1366 * 768][1].m_accountPoint = { { 537, 20 } };
	m_accountConfig[1920 * 1080][1].m_accountPoint = { { 537, 20 } };
	m_accountConfig[3840 * 2160][1].m_accountPoint = { { 537, 20 } };
	m_accountConfig[1920 * 1080][3].m_accountPoint = { { 537, 1057 }, { 599, 1058 }, { 659, 1059 } };

	m_accountConfig[1366 * 768][1].m_rightTopRect = { { 1315, 189, 1366, 249 } };
	m_accountConfig[1920 * 1080][1].m_rightTopRect = { { 1869, 189, 1920, 249 } };
	m_accountConfig[3840 * 2160][1].m_rightTopRect = { { 3789, 189, 3840, 249 } };
	m_accountConfig[1920 * 1080][3].m_rightTopRect = { { 1534, 169, 1654, 262 }, { 1211, 477, 1340, 569 },
		{ 1854, 486, 1920, 561 } };

	m_accountConfig[1366 * 768][1].m_clickTop = { { 455, 11 } };
	m_accountConfig[1920 * 1080][1].m_clickTop = { { 455, 11 } };
	m_accountConfig[3840 * 2160][1].m_clickTop = { { 455, 11 } };
	m_accountConfig[1920 * 1080][3].m_clickTop = { { 455, 11 }, { 123, 321 }, { 1738, 322 } };

	m_accountConfig[1366 * 768][1].m_talkHeroHeadPoint = { { 585, 131 } };
	m_accountConfig[1920 * 1080][1].m_talkHeroHeadPoint = { { 585, 131 } };
	m_accountConfig[3840 * 2160][1].m_talkHeroHeadPoint = { { 585, 131 } };
	m_accountConfig[1920 * 1080][3].m_talkHeroHeadPoint = { { 902, 139 }, { 590, 450 }, { 1223, 450 } };

	m_accountConfig[1366 * 768][1].m_talkWindowHeadRect = { { 354, 50, 990, 342 } };
	m_accountConfig[1920 * 1080][1].m_talkWindowHeadRect = { { 650, 77, 998, 552 } };
	m_accountConfig[3840 * 2160][1].m_talkWindowHeadRect = { { 1509, 636, 2224, 1076 } };
	m_accountConfig[1920 * 1080][3].m_talkWindowHeadRect = { { 650, 9, 998, 496 }, { 326, 318, 694, 719 },
		{ 992, 325, 1333, 757 } };

	m_accountConfig[1366 * 768][1].m_talkWindowHeadPath = "res\\talkhead.png";
	m_accountConfig[1920 * 1080][1].m_talkWindowHeadPath = "res\\talkhead.png";
	m_accountConfig[3840 * 2160][1].m_talkWindowHeadPath = "res\\talkhead.png";
	m_accountConfig[1920 * 1080][3].m_talkWindowHeadPath = "res\\talkhead.png";

	m_accountConfig[1366 * 768][1].m_goWindowHeadPath = "res\\gowindowhead2.png";
	m_accountConfig[1920 * 1080][1].m_goWindowHeadPath = "res\\gowindowhead.png";
	m_accountConfig[3840 * 2160][1].m_goWindowHeadPath = "res\\gowindowhead.png";
	m_accountConfig[1920 * 1080][3].m_goWindowHeadPath = "res\\gowindowhead.png";

	m_accountConfig[1366 * 768][1].m_chatPath = "res\\jiaohumaer.png";
	m_accountConfig[1920 * 1080][1].m_chatPath = "res\\jiaohumaer.png";
	m_accountConfig[3840 * 2160][1].m_chatPath = "res\\jiaohumaer.png";
	m_accountConfig[1920 * 1080][3].m_chatPath = "res\\jiaohumaer.png";
	
	m_accountConfig[1366 * 768][1].m_chatRect = { { 8, 462, 353, 668 } };
	m_accountConfig[1920 * 1080][1].m_chatRect = { { 0, 770, 348, 1009 } };
	m_accountConfig[3840 * 2160][1].m_chatRect = { { 35, 1856, 354, 2055 } };
	m_accountConfig[1920 * 1080][3].m_chatRect = { { 324, 465, 670, 677 }, { 0, 774, 350, 984 },
		{ 632, 772, 984, 988 } };
	
	m_accountConfig[1366 * 768][1].m_bloodRect = { { 571, 61, 826, 135 } };
	m_accountConfig[1920 * 1080][1].m_bloodRect = { { 571, 61, 826, 135 } };
	m_accountConfig[3840 * 2160][1].m_bloodRect = { { 571, 61, 826, 135 } };
	m_accountConfig[1920 * 1080][3].m_bloodRect = { { 897, 88, 1145, 143 }, { 566, 377, 827, 470 },
		{ 1202, 384, 1464, 460 } };

	m_accountConfig[1366 * 768][1].m_bloodPath = "res\\blood2.png";
	m_accountConfig[1920 * 1080][1].m_bloodPath = "res\\blood.png";
	m_accountConfig[3840 * 2160][1].m_bloodPath = "res\\blood.png";
	m_accountConfig[1920 * 1080][3].m_bloodPath = "res\\blood.png";

	m_accountConfig[1366 * 768][1].m_submitPoint = { { 1152, 285 } };
	m_accountConfig[1920 * 1080][1].m_submitPoint = { { 1713, 287 } };
	m_accountConfig[3840 * 2160][1].m_submitPoint = { { 3636, 289 } };
	m_accountConfig[1920 * 1080][3].m_submitPoint = { { 1398, 288 }, { 1072, 595 }, { 1708, 597 } };

	m_accountConfig[1366 * 768][1].m_intoGamePoint = { { 717, 628 } };
	m_accountConfig[1920 * 1080][1].m_intoGamePoint = { { 995, 938 } };
	m_accountConfig[3840 * 2160][1].m_intoGamePoint = { { 1960, 2019 } };
	m_accountConfig[1920 * 1080][3].m_intoGamePoint = { { 1024, 634 }, { 681, 943 }, { 1311, 940 } };

	m_accountConfig[1366 * 768][1].m_goPoint = { { 1355, 214 } };
	m_accountConfig[1920 * 1080][1].m_goPoint = { { 1910, 214 } };
	m_accountConfig[3840 * 2160][1].m_goPoint = { { 3831, 214 } };
	m_accountConfig[1920 * 1080][3].m_goPoint = { { 1592, 225 }, { 1274, 528 }, { 1902, 526 } };

	m_accountConfig[1366 * 768][1].m_goSleepTime = { 1500 };
	m_accountConfig[1920 * 1080][1].m_goSleepTime = { 200 };
	m_accountConfig[3840 * 2160][1].m_goSleepTime = { 200 };
	m_accountConfig[1920 * 1080][3].m_goSleepTime = { { 500, 500, 500 } };

	m_currentExePath = CSystem::GetCurrentExePath();
}

CConfig& CConfig::instance()
{
	static CConfig s_cconfig;
	return s_cconfig;
}