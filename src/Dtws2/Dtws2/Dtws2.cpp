#include "Dtws2.h"
//#include "11Controls/controls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"
#include "QtControls/COriginalButton.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "AccountTask.h"
#include "Point/PointAPI.h"
#include "FollowTask.h"
#include "HealTask.h"
#include "CHook/CHookAPI.h"

xyls::Point g_accountPoint[3] = { { 508, 1064 }, { 569, 1060 }, { 628, 1057 } };
int32_t g_accountCount = 3;
uint32_t* g_threadId = nullptr;

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
			CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->StopCurTask();
		}
		break;
		default:
			break;
		}
	}

	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

Dtws2::Dtws2(QWidget* parent):
	QMainWindow(parent),
	m_threadId(0),
	m_account(nullptr),
	m_follow(nullptr),
	m_heal(nullptr)
{
	ui.setupUi(this);
	init();
}

Dtws2::~Dtws2()
{

}

void Dtws2::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);

	m_threadId = CTaskThreadManager::Instance().Init();
	g_threadId = &m_threadId;

	CHook::Init(WH_KEYBOARD_LL, HookFun);

	m_account = new COriginalButton(this);
	m_account->setText(QStringLiteral("账号"));
	m_account->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_account, &COriginalButton::clicked, this, &Dtws2::onAccountButtonClicked);

	m_follow = new COriginalButton(this);
	m_follow->setText(QStringLiteral("跟随"));
	m_follow->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_follow, &COriginalButton::clicked, this, &Dtws2::onFollowButtonClicked);

	m_heal = new COriginalButton(this);
	m_heal->setText(QStringLiteral("补充"));
	m_heal->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_heal, &COriginalButton::clicked, this, &Dtws2::onHealButtonClicked);
}

bool Dtws2::check()
{
	return true;
}

void Dtws2::resizeEvent(QResizeEvent* eve)
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

void Dtws2::closeEvent(QCloseEvent* eve)
{
	CHook::Uninit();
	CTaskThreadManager::Instance().Uninit(m_threadId);
	m_threadId = 0;
	QMainWindow::closeEvent(eve);
}

void Dtws2::onAccountButtonClicked()
{
	showMinimized();
	std::shared_ptr<AccountTask> spAccountTask(new AccountTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spAccountTask);
}

void Dtws2::onFollowButtonClicked()
{
	showMinimized();
	std::shared_ptr<FollowTask> spFollowTask(new FollowTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spFollowTask);
}

void Dtws2::onHealButtonClicked()
{
	showMinimized();
	std::shared_ptr<HealTask> spHealTask(new HealTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spHealTask);
}