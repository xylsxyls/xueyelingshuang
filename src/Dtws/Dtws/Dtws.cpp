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

#define DTWS_SERVER_VERSION "1.0"

xyls::Point g_accountPoint[3] = { { 537, 1057 }, { 599, 1058 }, { 659, 1059 } };
int32_t g_accountCount = 1;
uint32_t* g_threadId = nullptr;
CStopWatch g_stopWatch;
bool g_altDown = false;
bool g_ctrlDown = false;
bool g_muqing = true;

LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码
	if (CHook::IsKeyDown(wParam))
	{
		DWORD code = CHook::GetVkCode(lParam);
		switch (code)
		{
		//delete键
		case 46:
		{
			//CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopCurTask();
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
	m_clientReceive(nullptr)
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

	m_threadId = CTaskThreadManager::Instance().Init();
	g_threadId = &m_threadId;

	if (computerName == "SC-202007261854" || computerName == "CN0614001185W")
	{
		CHook::Init(WH_KEYBOARD_LL, HookFun);
	}
	else if (computerName == "QQJJQPZPPWSYCVN")
	{
		g_accountCount = 3;
	}

	m_account = new COriginalButton(this);
	m_account->setText(QStringLiteral("账号"));
	m_account->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_account, &COriginalButton::clicked, this, &Dtws::onAccountButtonClicked);

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
	vecButton.push_back(m_follow);
	vecButton.push_back(m_heal);
	vecButton.push_back(m_followHeal);
	vecButton.push_back(m_skill);
	vecButton.push_back(m_water);
	vecButton.push_back(m_small);
	vecButton.push_back(m_muqing);

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
	std::shared_ptr<AccountTask> spAccountTask(new AccountTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spAccountTask);
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