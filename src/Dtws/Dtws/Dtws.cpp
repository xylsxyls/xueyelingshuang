#include "Dtws.h"
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
#include "CloseBeginTask.h"
#include "ClickLoginTask.h"
#include "Config.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"
#include "AssignThreadHelper.h"
#include "GoDestTask.h"
#include "ConvoyDestTask.h"

LRESULT WINAPI KeyboardHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码

	g_keyboard.acceptParam(wParam, lParam);

	if (g_config.m_isPrintVkCode)
	{
		RCSend("vkcode = %d", (int32_t)CHook::GetVkCode(lParam));
	}

	if (!g_config.m_hook || g_config.m_textWatch.GetWatchTime() < g_config.m_textWatchTime)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}
	
	if (g_keyboard.m_keyDown[DEL])
	{
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
	}
	else if (g_keyboard.m_keyDown['4'])
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_FOLLOW));
	}
	else if (g_keyboard.m_keyDown['T'])
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_HEAL));
	}
	else if (g_keyboard.m_keyDown[MINUS])
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_SUBMIT));
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
		AssignThreadManager::instance().stopAllTask();
		std::shared_ptr<SubmitTask> spTask(new SubmitTask);
		AssignThreadHelper::postEveryAssignTask(spTask);
	}

	if (g_keyboard.m_keyHasDown[CTRL])
	{
		if (g_keyboard.m_keyDown['2'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_ATTACK));
		}
		else if (g_keyboard.m_keyDown[KEY + '4'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_BINGJIANCUN));
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
			AssignThreadManager::instance().stopAllTask();

			std::shared_ptr<GoDestTask> spTask(new GoDestTask);
			std::vector<std::shared_ptr<CTask>> vecTask;
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				std::shared_ptr<GoDestTask> spAssignTask(new GoDestTask);
				spAssignTask->setAccountIndex(accountIndex);
				spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
					g_config.m_isBigLache ? 3 : 2, false);
				vecTask.push_back(spAssignTask);
			}
			AssignThreadManager::instance().postAssignTask(vecTask);
		}
		else if (g_keyboard.m_keyDown[KEY + '5'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_MUFENGLIN));
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
			AssignThreadManager::instance().stopAllTask();

			std::vector<std::shared_ptr<CTask>> vecTask;
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
				spAssignTask->setAccountIndex(accountIndex);
				spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
					g_config.m_isBigLache ? 0 : 0);
				vecTask.push_back(spAssignTask);
			}
			AssignThreadManager::instance().postAssignTask(vecTask);
		}
		else if (g_keyboard.m_keyDown[KEY + '6'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_HUILUODAO));
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
			AssignThreadManager::instance().stopAllTask();

			std::vector<std::shared_ptr<CTask>> vecTask;
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
				spAssignTask->setAccountIndex(accountIndex);
				spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
					g_config.m_isBigLache ? 1 : 1);
				vecTask.push_back(spAssignTask);
			}
			AssignThreadManager::instance().postAssignTask(vecTask);
		}
		else if (g_keyboard.m_keyDown[KEY + '7'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_LONGMENSHIKU));
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
			AssignThreadManager::instance().stopAllTask();

			std::vector<std::shared_ptr<CTask>> vecTask;
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
				spAssignTask->setAccountIndex(accountIndex);
				spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "bingleibiaojv",
					g_config.m_isBigLache ? 4 : 3);
				vecTask.push_back(spAssignTask);
			}
			AssignThreadManager::instance().postAssignTask(vecTask);
		}
		else if (g_keyboard.m_keyDown[KEY + '8'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_STOP));
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_DALEIGONG));
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
			AssignThreadManager::instance().stopAllTask();

			std::vector<std::shared_ptr<CTask>> vecTask;
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				std::shared_ptr<ConvoyDestTask> spAssignTask(new ConvoyDestTask);
				spAssignTask->setAccountIndex(accountIndex);
				spAssignTask->setParam(g_config.m_isBigLache ? "dongbeiyijun" : "pihuoshang",
					g_config.m_isBigLache ? 2 : 0);
				vecTask.push_back(spAssignTask);
			}
			AssignThreadManager::instance().postAssignTask(vecTask);
		}
	}
	if (g_keyboard.m_keyHasDown[ALT])
	{
		if (g_keyboard.m_keyDown['1'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_ESC));
		}
		else if (g_keyboard.m_keyDown['2'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_RISE));
		}
		else if (g_keyboard.m_keyDown['3'])
		{
			NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_PLANT));
		}
	}

	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

Dtws::Dtws(QWidget* parent):
	QMainWindow(parent),
	m_threadId(0),
	m_account(nullptr),
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

	g_config.m_textWatch.SetWatchTime(g_config.m_textWatchTime + 1000);

	m_taskThreadId = CTaskThreadManager::Instance().Init();
	g_config.m_taskThreadId = &m_taskThreadId;

	m_threadId = CTaskThreadManager::Instance().Init();
	g_config.m_threadId = &m_threadId;

	if (computerName == FIRST_COMPUTER || computerName == WORK_COMPUTER)
	{
		CHook::Init(WH_KEYBOARD_LL, KeyboardHookFun);
	}
	else if (computerName == THIRD_COMPUTER)
	{
		g_config.m_accountCount = 3;
	}

	if (computerName == SECOND_COMPUTER)
	{
		g_config.m_screenPixel = 1366 * 768;
	}

	AssignThreadManager::instance().init(g_config.m_accountCount);

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

	g_config.m_dtws = this;
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
	AssignThreadManager::instance().uninit();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
	CTaskThreadManager::Instance().Uninit(m_threadId);
	m_threadId = 0;
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
	spSleepTask2->setParam(g_config.m_accountCount == 3 ? 25000 : 1000);
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
	spSleepTask3->setParam(g_config.m_accountCount == 1 ?
		(CSystem::getComputerName() == SECOND_COMPUTER ? 35000 : 20000) : 30000);
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
	spSleepTask2->setParam(g_config.m_accountCount == 3 ? 25000 : 1000);
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
	spSleepTask3->setParam(g_config.m_accountCount == 1 ? 20000 : 30000);
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
	spSleepTask2->setParam(g_config.m_accountCount == 3 ? 25000 : 1000);
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
	spSleepTask3->setParam(g_config.m_accountCount == 1 ? 20000 : 30000);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSleepTask3);

	std::shared_ptr<CloseBeginTask> spCloseBeginTask(new CloseBeginTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spCloseBeginTask);
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
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
	AssignThreadManager::instance().stopAllTask();
	std::shared_ptr<WaterTask> spTask(new WaterTask);
	AssignThreadHelper::postEveryAssignTask(spTask);
}

void Dtws::onSmallButtonClicked()
{
	showMinimized();
	std::shared_ptr<SmallTask> spSmallTask(new SmallTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spSmallTask);
}

void Dtws::onMuqingButtonClicked()
{
	g_config.m_muqing = !g_config.m_muqing;
	m_muqing->setText(m_muqing ? QStringLiteral("沐晴凌云寨") : QStringLiteral("沐晴百花医"));
}

void Dtws::onJidiButtonClicked()
{
	showMinimized();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
	AssignThreadManager::instance().stopAllTask();

	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_JIDI));
	}

	std::vector<std::shared_ptr<CTask>> vecTask;
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		std::shared_ptr<GoDestTask> spAssignTask(new GoDestTask);
		spAssignTask->setAccountIndex(accountIndex);
		spAssignTask->setParam("ganquangujieyinren", 0, true);
		vecTask.push_back(spAssignTask);
	}
	AssignThreadManager::instance().postAssignTask(vecTask);
}

void Dtws::onChangshougongButtonClicked()
{
	showMinimized();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_taskThreadId)->StopAllTask();
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->StopAllTask();
	AssignThreadManager::instance().stopAllTask();

	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(DTWS_CHANGSHOUGONG));
	}

	std::vector<std::shared_ptr<CTask>> vecTask;
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		std::shared_ptr<GoDestTask> spAssignTask(new GoDestTask);
		spAssignTask->setAccountIndex(accountIndex);
		spAssignTask->setParam("pingkouzhenren", 0, false);
		vecTask.push_back(spAssignTask);
	}
	AssignThreadManager::instance().postAssignTask(vecTask);
}

void Dtws::onLacheButtonClicked()
{
	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		g_config.m_isBigLache = !g_config.m_isBigLache;
		m_lache->setText(g_config.m_isBigLache ? QStringLiteral("大车") : QStringLiteral("小车"));
		NetSender::instance().sendServer(PROJECT_DTWS, std::to_string(g_config.m_isBigLache ? DTWS_DACHE : DTWS_XIAOCHE));
	}
}

void Dtws::onChangeLacheText()
{
	m_lache->setText(g_config.m_isBigLache ? QStringLiteral("大车") : QStringLiteral("小车"));
}