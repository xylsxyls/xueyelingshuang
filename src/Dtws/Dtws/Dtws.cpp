#include "Dtws.h"
//#include "11Controls/controls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"
#include "QtControls/COriginalButton.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "AccountTask.h"
#include "Point/PointAPI.h"
#include "FollowTask.h"
#include "HealTask.h"
#include "SkillTask.h"
#include "CHook/CHookAPI.h"
#include "ClientReceive.h"
#include "CSystem/CSystemAPI.h"
#include "FiveTask.h"
#include "ATask.h"
#include "WaterTask.h"
#include "SmallTask.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "QtControls/ComboBox.h"
#include "Rect/RectAPI.h"
#include "AssignThreadTask.h"
#include "ConvoyTask.h"
#include "GoFindClickTask.h"
#include "SubmitTask.h"
#include "DtwsParam.h"
#include "StartTask.h"
#include "SleepTask.h"
#include "CloseBeginTask.h"
#include "ClickLoginTask.h"

xyls::Point g_accountPoint[3] = { { 537, 1057 }, { 599, 1058 }, { 659, 1059 } };
xyls::Rect g_rightTopRect[3] = { { 1534, 169, 1654, 262 }, { 1211, 477, 1340, 569 }, { 1854, 486, 1920, 561 } };
xyls::Point g_clickTop[3] = { { 455, 11 }, { 123, 321 }, { 1738, 322 } };
xyls::Point g_talkPoint[3] = { { 902, 139 }, { 590, 450 }, { 1223, 450 } };
xyls::Rect g_talkheadRect[3] = { { 650, 9, 998, 496 }, { 326, 318, 694, 719 }, { 992, 325, 1333, 757 } };
xyls::Rect g_chatRect[3] = { { 324, 465, 670, 677 }, { 0, 774, 350, 984 }, { 632, 772, 984, 988 } };
xyls::Rect g_bloodRect[3] = { { 571, 61, 1156, 149 }, { 566, 377, 827, 470 }, { 1202, 384, 1464, 460 } };
xyls::Point g_taskPoint[3] = { { 1398, 288 }, { 1072, 595 }, { 1708, 597 } };
xyls::Point g_intoGamePoint[3] = { { 1024, 634 }, { 681, 943 }, { 1311, 940 } };
xyls::Point g_accept = { 63, 418 };
xyls::Point g_get = { 64, 326 };
int32_t g_accountCount = 1;
uint32_t* g_taskThreadId = nullptr;
uint32_t* g_threadId = nullptr;
CStopWatch g_stopWatch;
bool g_altDown = false;
bool g_ctrlDown = false;
bool g_muqing = true;
bool g_hook = true;
bool g_isBigLache = true;
Dtws* g_dtws = nullptr;


LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码
	if (!g_hook)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}
	if (CHook::IsKeyDown(wParam))
	{
		DWORD code = CHook::GetVkCode(lParam);
		//RCSend("code = %d", code);
		switch (code)
		{
		//delete键
		case 46:
		{
			CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
		}
		break;
		//alt键
		case 164:
		{
			g_altDown = true;
		}
		break;
		//ctrl键
		case 162:
		{
			g_ctrlDown = true;
		}
		break;
		case '2':
		{
			if (g_altDown)
			{
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_RISE));
			}
			if (g_ctrlDown)
			{
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_ATTACK));
			}
		}
		break;
		case '4':
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_FOLLOW));
		}
		break;
		case 'T':
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_HEAL));
		}
		break;
		case '5':
		{
			if (g_stopWatch.GetWatchTime() > 10000)
			{
				g_stopWatch.SetWatchTime(0);
				std::shared_ptr<FiveTask> spFiveTask(new FiveTask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spFiveTask);
			}
		}
		break;
		case 'A':
		{
			if (g_stopWatch.GetWatchTime() > 10000)
			{
				g_stopWatch.SetWatchTime(0);
				std::shared_ptr<ATask> spATask(new ATask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spATask);
			}
		}
		break;
		case 100:
		{
			if (g_stopWatch.GetWatchTime() > 5000)
			{
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

				std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
				std::vector<std::shared_ptr<CTask>> vecSpDoTask;
				int32_t clientIndex = -1;
				while (clientIndex++ != g_accountCount - 1)
				{
					GoFindClickTask* goFindClickTask = new GoFindClickTask;
					goFindClickTask->setParam(600,
						clientIndex,
						g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
						g_isBigLache ? 3 : 2,
						std::vector<xyls::Point>());
					std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
					vecSpDoTask.push_back(spGoFindClickTask);
				}
				spAssignThreadTask->setParam(vecSpDoTask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_BINGJIANCUN));
			}
		}
		break;
		case 101:
		{
			if (g_stopWatch.GetWatchTime() > 5000)
			{
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

				std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
				std::vector<std::shared_ptr<CTask>> vecSpDoTask;
				int32_t clientIndex = -1;
				while (clientIndex++ != g_accountCount - 1)
				{
					ConvoyTask* convoyTask = new ConvoyTask;
					std::vector<xyls::Point> vecAcceptPoint;
					vecAcceptPoint.push_back(g_get);
					vecAcceptPoint.push_back(g_accept);
					convoyTask->setParam(600,
						clientIndex,
						g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
						g_isBigLache ? 0 : 0,
						vecAcceptPoint);
					std::shared_ptr<CTask> spConvoyTask(convoyTask);
					vecSpDoTask.push_back(spConvoyTask);
				}
				spAssignThreadTask->setParam(vecSpDoTask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_MUFENGLIN));
			}
		}
		break;
		case 102:
		{
			if (g_stopWatch.GetWatchTime() > 5000)
			{
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

				std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
				std::vector<std::shared_ptr<CTask>> vecSpDoTask;
				int32_t clientIndex = -1;
				while (clientIndex++ != g_accountCount - 1)
				{
					ConvoyTask* convoyTask = new ConvoyTask;
					std::vector<xyls::Point> vecAcceptPoint;
					vecAcceptPoint.push_back(g_get);
					vecAcceptPoint.push_back(g_accept);
					convoyTask->setParam(600,
						clientIndex,
						g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
						g_isBigLache ? 1 : 1,
						vecAcceptPoint);
					std::shared_ptr<CTask> spConvoyTask(convoyTask);
					vecSpDoTask.push_back(spConvoyTask);
				}
				spAssignThreadTask->setParam(vecSpDoTask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_HUILUODAO));
			}
		}
		break;
		case 103:
		{
			if (g_stopWatch.GetWatchTime() > 5000)
			{
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

				std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
				std::vector<std::shared_ptr<CTask>> vecSpDoTask;
				int32_t clientIndex = -1;
				while (clientIndex++ != g_accountCount - 1)
				{
					ConvoyTask* convoyTask = new ConvoyTask;
					std::vector<xyls::Point> vecAcceptPoint;
					vecAcceptPoint.push_back(g_get);
					vecAcceptPoint.push_back(g_accept);
					convoyTask->setParam(600,
						clientIndex,
						g_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
						g_isBigLache ? 4 : 3,
						vecAcceptPoint);
					std::shared_ptr<CTask> spConvoyTask(convoyTask);
					vecSpDoTask.push_back(spConvoyTask);
				}
				spAssignThreadTask->setParam(vecSpDoTask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_LONGMENSHIKU));
			}
		}
		break;
		case 104:
		{
			if (g_stopWatch.GetWatchTime() > 5000)
			{
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
				CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

				std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
				std::vector<std::shared_ptr<CTask>> vecSpDoTask;
				int32_t clientIndex = -1;
				while (clientIndex++ != g_accountCount - 1)
				{
					ConvoyTask* convoyTask = new ConvoyTask;
					std::vector<xyls::Point> vecAcceptPoint;
					vecAcceptPoint.push_back(g_accept);
					vecAcceptPoint.push_back(g_accept);
					convoyTask->setParam(600,
						clientIndex,
						g_isBigLache ? "dongbeiyijun" : "pihuoshang",
						g_isBigLache ? 2 : 0,
						vecAcceptPoint);
					std::shared_ptr<CTask> spConvoyTask(convoyTask);
					vecSpDoTask.push_back(spConvoyTask);
				}
				spAssignThreadTask->setParam(vecSpDoTask);
				CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
				NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_DALEIGONG));
			}
		}
		break;
		case 109:
		{
			CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

			std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
			std::vector<std::shared_ptr<CTask>> vecSpDoTask;
			int32_t clientIndex = -1;
			while (clientIndex++ != g_accountCount - 1)
			{
				SubmitTask* submitTask = new SubmitTask;
				submitTask->setParam(500);
				std::shared_ptr<CTask> spSubmitTask(submitTask);
				vecSpDoTask.push_back(spSubmitTask);
			}
			spAssignThreadTask->setParam(vecSpDoTask);
			CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_SUBMIT));
		}
		break;
		default:
			break;
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{
		DWORD code = CHook::GetVkCode(lParam);
		switch (code)
		{
		//alt键
		case 164:
		{
			g_altDown = false;
		}
		break;
		//ctrl键
		case 162:
		{
			g_ctrlDown = false;
		}
		break;
		default:
			break;
		}
	}

	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

Dtws::Dtws(QWidget* parent):
	QMainWindow(parent),
	m_threadId(0),
	m_account(nullptr),
	m_follow(nullptr),
	m_heal(nullptr),
	m_followHeal(nullptr),
	m_skill(nullptr),
	m_water(nullptr),
	m_small(nullptr),
	m_muqing(nullptr),
	m_jidi(nullptr),
	m_lache(nullptr),
	m_clientReceive(nullptr),
	m_account2(nullptr),
	m_account3(nullptr)
{
	ui.setupUi(this);
	init();
}

Dtws::~Dtws()
{

}

void Dtws::init()
{
	if (!check())
	{
		return;
	}

	std::string computerName = CSystem::getComputerName();
	m_clientReceive = new ClientReceive;
	NetSender::instance().initClientReceive(m_clientReceive);
	ProcessWork::instance().initReceive();
	NetSender::instance().initClient(PROJECT_DTWS, std::string("DtwsServer") + DTWS_SERVER_VERSION, computerName);

	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);

	m_taskThreadId = CTaskThreadManager::Instance().Init();
	g_taskThreadId = &m_taskThreadId;

	m_threadId = CTaskThreadManager::Instance().Init();
	g_threadId = &m_threadId;

	if (computerName == FIRST_COMPUTER || computerName == WORK_COMPUTER)
	{
		CHook::Init(WH_KEYBOARD_LL, HookFun);
	}
	else if (computerName == THIRD_COMPUTER)
	{
		g_accountCount = 3;
	}

	m_account = new COriginalButton(this);
	m_account->setText(QStringLiteral("账号"));
	m_account->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_account, &COriginalButton::clicked, this, &Dtws::onAccountButtonClicked);

	m_account2 = new COriginalButton(this);
	m_account2->setText(QStringLiteral("第二批"));
	m_account2->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_account2, &COriginalButton::clicked, this, &Dtws::onAccount2ButtonClicked);

	m_account3 = new COriginalButton(this);
	m_account3->setText(QStringLiteral("第三批"));
	m_account3->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_account3, &COriginalButton::clicked, this, &Dtws::onAccount3ButtonClicked);

	m_follow = new COriginalButton(this);
	m_follow->setText(QStringLiteral("跟随"));
	m_follow->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_follow, &COriginalButton::clicked, this, &Dtws::onFollowButtonClicked);

	m_heal = new COriginalButton(this);
	m_heal->setText(QStringLiteral("补充"));
	m_heal->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_heal, &COriginalButton::clicked, this, &Dtws::onHealButtonClicked);

	m_followHeal = new COriginalButton(this);
	m_followHeal->setText(QStringLiteral("跟随补充"));
	m_followHeal->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_followHeal, &COriginalButton::clicked, this, &Dtws::onFollowHealButtonClicked);

	m_skill = new COriginalButton(this);
	m_skill->setText(QStringLiteral("技能"));
	m_skill->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_skill, &COriginalButton::clicked, this, &Dtws::onSkillButtonClicked);

	m_water = new COriginalButton(this);
	m_water->setText(QStringLiteral("采水"));
	m_water->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_water, &COriginalButton::clicked, this, &Dtws::onWaterButtonClicked);

	m_small = new COriginalButton(this);
	m_small->setText(QStringLiteral("缩小"));
	m_small->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_small, &COriginalButton::clicked, this, &Dtws::onSmallButtonClicked);

	m_muqing = new COriginalButton(this);
	m_muqing->setText(QStringLiteral("沐晴凌云寨"));
	m_muqing->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_muqing, &COriginalButton::clicked, this, &Dtws::onMuqingButtonClicked);

	m_jidi = new COriginalButton(this);
	m_jidi->setText(QStringLiteral("基地"));
	m_jidi->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_jidi, &COriginalButton::clicked, this, &Dtws::onJidiButtonClicked);

	m_changshougong = new COriginalButton(this);
	m_changshougong->setText(QStringLiteral("长寿宫"));
	m_changshougong->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_changshougong, &COriginalButton::clicked, this, &Dtws::onChangshougongButtonClicked);

	m_lache = new COriginalButton(this);
	m_lache->setText(QStringLiteral("大车"));
	m_lache->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_lache, &COriginalButton::clicked, this, &Dtws::onLacheButtonClicked);
	QObject::connect(this, &Dtws::changeLacheText, this, &Dtws::onChangeLacheText, Qt::QueuedConnection);

	g_dtws = this;
}

bool Dtws::check()
{
	return true;
}

void Dtws::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	std::vector<COriginalButton*> vecButton;
	vecButton.push_back(m_account);
	vecButton.push_back(m_account2);
	vecButton.push_back(m_account3);
	vecButton.push_back(m_follow);
	vecButton.push_back(m_heal);
	vecButton.push_back(m_followHeal);
	vecButton.push_back(m_skill);
	vecButton.push_back(m_water);
	vecButton.push_back(m_small);
	vecButton.push_back(m_muqing);
	vecButton.push_back(m_jidi);
	vecButton.push_back(m_changshougong);
	vecButton.push_back(m_lache);

	int32_t cowCount = 4;
	int32_t width = 140;
	int32_t height = 30;
	int32_t space = 10;

	int32_t index = -1;
	while (index++ != vecButton.size() - 1)
	{
		vecButton[index]->setGeometry(index % cowCount * (width + space), index / cowCount * (height + space), width, height);
	}
}

void Dtws::closeEvent(QCloseEvent* eve)
{
	CHook::Uninit();
	CTaskThreadManager::Instance().Uninit(m_taskThreadId);
	m_taskThreadId = 0;
	CTaskThreadManager::Instance().Uninit(m_threadId);
	m_threadId = 0;
	ProcessWork::instance().uninitReceive();
	delete m_clientReceive;
	m_clientReceive = nullptr;
	QMainWindow::closeEvent(eve);
}

void Dtws::onAccountButtonClicked()
{
	showMinimized();
	std::shared_ptr<SleepTask> spSleepTask1(new SleepTask);
	spSleepTask1->setParam(1500);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask1);

	std::shared_ptr<StartTask> spStartTask(new StartTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spStartTask);

	std::shared_ptr<SleepTask> spSleepTask2(new SleepTask);
	spSleepTask2->setParam(g_accountCount == 1 ? 22000 : 25000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask2);

	std::shared_ptr<ClickLoginTask> spClickLoginTask(new ClickLoginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spClickLoginTask);

	std::shared_ptr<SmallTask> spSmallTask(new SmallTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSmallTask);

	std::vector<std::pair<std::string, std::string>> vecAccount;
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2shaolin", "yangnan"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2hanbingmen", "yangnan"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2xiayindao", "yangnan"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2baihuaguxi", "yangnan"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2baihuayixi", "yangnan"));
	std::shared_ptr<AccountTask> spAccountTask(new AccountTask);
	spAccountTask->setParam(vecAccount);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spAccountTask);

	std::shared_ptr<SleepTask> spSleepTask3(new SleepTask);
	spSleepTask3->setParam(g_accountCount == 1 ? 20000 : 30000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask3);

	std::shared_ptr<CloseBeginTask> spCloseBeginTask(new CloseBeginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCloseBeginTask);
}

void Dtws::onAccount2ButtonClicked()
{
	showMinimized();
	std::shared_ptr<SleepTask> spSleepTask1(new SleepTask);
	spSleepTask1->setParam(1500);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask1);

	std::shared_ptr<StartTask> spStartTask(new StartTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spStartTask);

	std::shared_ptr<SleepTask> spSleepTask2(new SleepTask);
	spSleepTask2->setParam(g_accountCount == 1 ? 22000 : 25000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask2);

	std::shared_ptr<ClickLoginTask> spClickLoginTask(new ClickLoginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spClickLoginTask);

	std::shared_ptr<SmallTask> spSmallTask(new SmallTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSmallTask);

	std::vector<std::pair<std::string, std::string>> vecAccount;
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2shaolin22", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2hanbingmen22", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2xiayindao22", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2baihuaguxi22", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2baihuayixi22", "Yangnan8"));
	std::shared_ptr<AccountTask> spAccountTask(new AccountTask);
	spAccountTask->setParam(vecAccount);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spAccountTask);

	std::shared_ptr<SleepTask> spSleepTask3(new SleepTask);
	spSleepTask3->setParam(g_accountCount == 1 ? 20000 : 30000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask3);

	std::shared_ptr<CloseBeginTask> spCloseBeginTask(new CloseBeginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCloseBeginTask);
}

void Dtws::onAccount3ButtonClicked()
{
	showMinimized();
	std::shared_ptr<SleepTask> spSleepTask1(new SleepTask);
	spSleepTask1->setParam(1500);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask1);

	std::shared_ptr<StartTask> spStartTask(new StartTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spStartTask);

	std::shared_ptr<SleepTask> spSleepTask2(new SleepTask);
	spSleepTask2->setParam(g_accountCount == 1 ? 22000 : 25000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask2);

	std::shared_ptr<ClickLoginTask> spClickLoginTask(new ClickLoginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spClickLoginTask);

	std::shared_ptr<SmallTask> spSmallTask(new SmallTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSmallTask);

	std::vector<std::pair<std::string, std::string>> vecAccount;
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2shaolin33", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2tianshameng", "yangnan"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2xiayindao33", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2baihuaguxi33", "Yangnan8"));
	vecAccount.push_back(std::pair<std::string, std::string>("dtws2baihuayixi33", "Yangnan8"));
	std::shared_ptr<AccountTask> spAccountTask(new AccountTask);
	spAccountTask->setParam(vecAccount);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spAccountTask);

	std::shared_ptr<SleepTask> spSleepTask3(new SleepTask);
	spSleepTask3->setParam(g_accountCount == 1 ? 20000 : 30000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask3);

	std::shared_ptr<CloseBeginTask> spCloseBeginTask(new CloseBeginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCloseBeginTask);
}

void Dtws::onFollowButtonClicked()
{
	showMinimized();
	std::shared_ptr<FollowTask> spFollowTask(new FollowTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spFollowTask);
}

void Dtws::onHealButtonClicked()
{
	showMinimized();
	std::shared_ptr<HealTask> spHealTask(new HealTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spHealTask);
}

void Dtws::onFollowHealButtonClicked()
{
	showMinimized();
	std::shared_ptr<FollowTask> spFollowTask(new FollowTask);
	spFollowTask->setParam(true);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spFollowTask);
}

void Dtws::onSkillButtonClicked()
{
	showMinimized();
	std::shared_ptr<SkillTask> spSkillTask(new SkillTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSkillTask);
}

void Dtws::onWaterButtonClicked()
{
	showMinimized();
	std::shared_ptr<WaterTask> spWaterTask(new WaterTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spWaterTask);
}

void Dtws::onSmallButtonClicked()
{
	showMinimized();
	std::shared_ptr<SmallTask> spSmallTask(new SmallTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSmallTask);
}

void Dtws::onMuqingButtonClicked()
{
	g_muqing = !g_muqing;
	m_muqing->setText(g_muqing ? QStringLiteral("沐晴凌云寨") : QStringLiteral("沐晴百花医"));
}

void Dtws::onJidiButtonClicked()
{
	showMinimized();
	CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
	CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

	std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
	std::vector<std::shared_ptr<CTask>> vecSpDoTask;
	int32_t clientIndex = -1;
	while (clientIndex++ != g_accountCount - 1)
	{
		GoFindClickTask* goFindClickTask = new GoFindClickTask;
		std::vector<xyls::Point> vecAcceptPoint;
		vecAcceptPoint.push_back(g_accept);
		goFindClickTask->setParam(1000, clientIndex, "ganquangujieyinren", 0, vecAcceptPoint);
		std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
		vecSpDoTask.push_back(spGoFindClickTask);
	}
	spAssignThreadTask->setParam(vecSpDoTask);
	CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_JIDI));
	}
}

void Dtws::onChangshougongButtonClicked()
{
	showMinimized();
	CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->StopAllTask();
	CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopAllTask();

	std::shared_ptr<AssignThreadTask> spAssignThreadTask(new AssignThreadTask);
	std::vector<std::shared_ptr<CTask>> vecSpDoTask;
	int32_t clientIndex = -1;
	while (clientIndex++ != g_accountCount - 1)
	{
		GoFindClickTask* goFindClickTask = new GoFindClickTask;
		goFindClickTask->setParam(1000, clientIndex, "pingkouzhenren", 0, std::vector<xyls::Point>());
		std::shared_ptr<CTask> spGoFindClickTask(goFindClickTask);
		vecSpDoTask.push_back(spGoFindClickTask);
	}
	spAssignThreadTask->setParam(vecSpDoTask);
	CTaskThreadManager::Instance().GetThreadInterface(*g_taskThreadId)->PostTask(spAssignThreadTask);
	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_CHANGSHOUGONG));
	}
}

void Dtws::onLacheButtonClicked()
{
	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		g_isBigLache = !g_isBigLache;
		m_lache->setText(g_isBigLache ? QStringLiteral("大车") : QStringLiteral("小车"));
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(g_isBigLache ? DTWS_DACHE : DTWS_XIAOCHE));
	}
}

void Dtws::onChangeLacheText()
{
	m_lache->setText(g_isBigLache ? QStringLiteral("大车") : QStringLiteral("小车"));
}