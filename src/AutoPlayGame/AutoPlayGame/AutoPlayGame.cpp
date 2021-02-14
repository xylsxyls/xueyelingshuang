#include "AutoPlayGame.h"
//#include "11Controls/controls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"
#include "QtControls/ComboBox.h"
#include "QtControls/COriginalButton.h"
#include "AutoPlayGameTask.h"
#include "CHook/CHookAPI.h"

uint32_t* g_playThreadId = nullptr;

LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码

	const DWORD& vkCode = CHook::GetVkCode(lParam);

	if (CHook::IsKeyDown(wParam))
	{
		//F12
		if (vkCode == 123)
		{
			CTaskThreadManager::Instance().Uninit(*g_playThreadId);
			*g_playThreadId = CTaskThreadManager::Instance().Init();
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{

	}

	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

AutoPlayGame::AutoPlayGame(QWidget* parent):
	QMainWindow(parent),
	m_group(nullptr),
	m_hero(nullptr),
	m_scene(nullptr),
	m_teammate1(nullptr),
	m_teammate2(nullptr),
	m_begin(nullptr),
	m_end(nullptr),
	m_playThreadId(0)
{
	ui.setupUi(this);
	//m_button = new COriginalButton(this);
	init();
}

AutoPlayGame::~AutoPlayGame()
{
	CTaskThreadManager::Instance().Uninit(m_playThreadId);
	m_playThreadId = 0;
}

void AutoPlayGame::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	//m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	//QObject::connect(m_button, &COriginalButton::clicked, this, &AutoPlayGame::onButtonClicked);

	m_group = new ComboBox(this);
	m_group->addItem(QStringLiteral("蓝方"));
	m_group->addItem(QStringLiteral("紫方"));
	m_group->setHidden(true);

	m_hero = new ComboBox(this);
	m_hero->addItem(QStringLiteral("刀锋"));
	m_hero->addItem(QStringLiteral("猫"));

	m_scene = new ComboBox(this);
	m_scene->addItem(QStringLiteral("游戏内"));
	m_scene->addItem(QStringLiteral("游戏外"));

	m_teammate1 = new ComboBox(this);
	m_teammate1->addItem(QStringLiteral("F1"));
	m_teammate1->addItem(QStringLiteral("F2"));
	m_teammate1->addItem(QStringLiteral("F3"));
	m_teammate1->addItem(QStringLiteral("F4"));
	m_teammate1->addItem(QStringLiteral("F5"));
	m_teammate1->setCurrentIndex(1);

	m_teammate2 = new ComboBox(this);
	m_teammate2->addItem(QStringLiteral("F1"));
	m_teammate2->addItem(QStringLiteral("F2"));
	m_teammate2->addItem(QStringLiteral("F3"));
	m_teammate2->addItem(QStringLiteral("F4"));
	m_teammate2->addItem(QStringLiteral("F5"));
	m_teammate2->setCurrentIndex(2);
	
	m_begin = new COriginalButton(this);
	m_begin->setText(QStringLiteral("开始"));
	m_begin->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_begin, &COriginalButton::clicked, this, &AutoPlayGame::onBeginClicked);

	CHook::Init(WH_KEYBOARD_LL, HookFun);

	m_end = new COriginalButton(this);
	m_end->setText(QStringLiteral("结束"));
	m_end->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_end, &COriginalButton::clicked, this, &AutoPlayGame::onEndClicked);

	g_playThreadId = &m_playThreadId;
	m_playThreadId = CTaskThreadManager::Instance().Init();
}

bool AutoPlayGame::check()
{
	return true;
}

void AutoPlayGame::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_scene->setGeometry(50, 50, 100, 30);
	m_hero->setGeometry(50, 150, 100, 30);
	m_group->setGeometry(250, 150, 100, 30);
	m_teammate1->setGeometry(50, 250, 100, 30);
	m_teammate2->setGeometry(250, 250, 100, 30);
	m_begin->setGeometry(400, 80, 150, 60);
	m_end->setGeometry(400, 260, 150, 60);
}

void AutoPlayGame::closeEvent(QCloseEvent* eve)
{
	CTaskThreadManager::Instance().Uninit(m_playThreadId);
	m_playThreadId = 0;
	CHook::Uninit();
	QMainWindow::closeEvent(eve);
}

void AutoPlayGame::onBeginClicked()
{
	std::shared_ptr<AutoPlayGameTask> spAutoPlayGameTask(new AutoPlayGameTask);
	spAutoPlayGameTask->setParam(CStringManager::UnicodeToAnsi(m_hero->currentText().toStdWString()),
		CStringManager::UnicodeToAnsi(m_scene->currentText().toStdWString()),
		CStringManager::UnicodeToAnsi(m_teammate1->currentText().toStdWString()),
		CStringManager::UnicodeToAnsi(m_teammate2->currentText().toStdWString()));
	CTaskThreadManager::Instance().GetThreadInterface(m_playThreadId)->PostTask(spAutoPlayGameTask);
}

void AutoPlayGame::onEndClicked()
{
	CTaskThreadManager::Instance().Uninit(m_playThreadId);
	m_playThreadId = CTaskThreadManager::Instance().Init();
}