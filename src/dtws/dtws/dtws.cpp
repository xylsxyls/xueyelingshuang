#include "dtws.h"
#include "QtControls/COriginalButton.h"
#include "DialogManager/DialogManagerAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LoginTask.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "GlobalParam.h"
#include "ClearTask.h"
#include "MainTask.h"
#include "CheckTask.h"
#include "FightTask.h"
#include "ClickTask.h"
#include "TieQiangTask.h"
#include "TuLangTask.h"
#include "WaiZuiTask.h"
#include "FengMingShanTask.h"
#include "CollectTask.h"

dtws::dtws(QWidget* parent):
QMainWindow(parent),
m_workThreadId(0),
m_checkWorkThreadId(0),
m_workThread(nullptr),
m_checkWorkThread(nullptr)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	init();
}

dtws::~dtws()
{

}

void dtws::init()
{
	if (!check())
	{
		return;
	}
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &dtws::onButtonClicked);

	m_workThreadId = CTaskThreadManager::Instance().Init();
	m_checkWorkThreadId = CTaskThreadManager::Instance().Init();
	m_workThread = CTaskThreadManager::Instance().GetThreadInterface(m_workThreadId);
	m_checkWorkThread = CTaskThreadManager::Instance().GetThreadInterface(m_checkWorkThreadId);
}

bool dtws::check()
{
	return true;
	return m_button != nullptr;
}

void dtws::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_button->setGeometry(360, 120, 160, 80);
}

void dtws::closeEvent(QCloseEvent* eve)
{
	CTaskThreadManager::Instance().UninitAll();
}

void dtws::onButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_editTip = QStringLiteral("请输入开始步骤");
	inputDialogParam.m_defaultText = QStringLiteral("1");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	Sleep(500);

	std::shared_ptr<CheckTask> spCheckTask(new CheckTask);
	m_checkWorkThread->PostTask(spCheckTask);

	int32_t step = atoi(inputDialogParam.m_editText.toStdString().c_str());

	switch (step)
	{
	case 1:
	{
		GLOBAL_CONFIG[USER] = "dtws2baihuaguxi@163.com";
		GLOBAL_CONFIG[PASSWORD] = "yangnan";
		std::shared_ptr<LoginTask> spLoginTask(new LoginTask);
		spLoginTask->setStep(1);
		spLoginTask->setParam(GLOBAL_CONFIG[USER].toString(), GLOBAL_CONFIG[PASSWORD].toString());
		m_workThread->PostTask(spLoginTask);
	}
	case 2:
	{
		std::shared_ptr<ClearTask> spClearTask(new ClearTask);
		spClearTask->setStep(2);
		m_workThread->PostTask(spClearTask);
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(2);
		spFightTask->setParam({ 1741, 286 }, 2000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 3:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(3);
		m_workThread->PostTask(spMainTask);
	}
	case 4:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(4);
		spFightTask->setParam({ 1741, 286 }, 10000, 6, 5, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 5:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(5);
		m_workThread->PostTask(spMainTask);
	}
	case 6:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(6);
		spFightTask->setParam({ 1741, 286 }, 25000, 6, 5, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 7:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(7);
		m_workThread->PostTask(spMainTask);
	}
	case 8:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(8);
		spFightTask->setParam({ 1741, 286 }, 40000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 9:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(9);
		m_workThread->PostTask(spMainTask);
	}
	case 10:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(10);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 100));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(904, 414), 100));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(903, 544), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1207, 260), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 3000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 11:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(11);
		m_workThread->PostTask(spMainTask);
	}
	case 12:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(12);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1703, 136), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(860, 251), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1179, 360), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1179, 923), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1271, 202), 2000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 13:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(13);
		m_workThread->PostTask(spMainTask);
	}
	case 14:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(14);
		spFightTask->setParam({ 1741, 286 }, 20000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 15:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(15);
		m_workThread->PostTask(spMainTask);
	}
	case 16:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(16);
		spFightTask->setParam({ 1741, 286 }, 10000, 40, 40, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 17:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(17);
		m_workThread->PostTask(spMainTask);
	}
	case 18:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(18);
		spFightTask->setParam({ 1741, 286 }, 10000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 19:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(19);
		m_workThread->PostTask(spMainTask);
	}
	case 20:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(20);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1699, 140), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(854, 250), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1174, 408), 2000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1174, 925), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1174, 925), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1174, 925), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1174, 925), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1273, 198), 500));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 21:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(21);
		m_workThread->PostTask(spMainTask);
	}
	case 22:
	{
		std::shared_ptr<ClickTask> spClickTask1(new ClickTask);
		spClickTask1->setStep(22);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick1;
		vecClick1.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 500));
		vecClick1.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(985, 477), 18000));
		vecClick1.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(981, 471), 5000));
		spClickTask1->setParam(m_checkWorkThreadId, vecClick1);
		m_workThread->PostTask(spClickTask1);

		std::shared_ptr<CollectTask> spCollectTask(new CollectTask);
		m_workThread->PostTask(spCollectTask);

		std::shared_ptr<ClickTask> spClickTask2(new ClickTask);
		spClickTask2->setStep(22);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick2;
		vecClick2.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(981, 471), 5000));
		vecClick2.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 500));
		vecClick2.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 18000));
		spClickTask2->setParam(m_checkWorkThreadId, vecClick2);
		m_workThread->PostTask(spClickTask2);
	}
	case 23:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(23);
		m_workThread->PostTask(spMainTask);
	}
	case 24:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(24);
		spFightTask->setParam({ 1741, 286 }, 20000, 5, 5, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 25:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(25);
		m_workThread->PostTask(spMainTask);
	}
	case 26:
	{
		std::shared_ptr<TieQiangTask> spTieQiangTask(new TieQiangTask);
		spTieQiangTask->setStep(26);
		m_workThread->PostTask(spTieQiangTask);
	}
	case 27:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(27);
		spFightTask->setParam({ 1766, 317 }, 1000, 0, 0, { 1766, 317 });
		m_workThread->PostTask(spFightTask);
	}
	case 28:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(28);
		m_workThread->PostTask(spMainTask);
	}
	case 29:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(29);
		spFightTask->setParam({ 1741, 286 }, 10000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 30:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(30);
		m_workThread->PostTask(spMainTask);
	}
	case 31:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(31);
		spFightTask->setParam({ 1741, 286 }, 10000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 32:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(32);
		m_workThread->PostTask(spMainTask);
	}
	case 33:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(33);
		spFightTask->setParam({ 1741, 286 }, 10000, 5, 13, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 34:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(34);
		m_workThread->PostTask(spMainTask);
	}
	case 35:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(35);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(871, 698), 10000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(876, 698), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(871, 698), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(876, 698), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 10000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 36:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(36);
		m_workThread->PostTask(spMainTask);
	}
	case 37:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(37);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(871, 698), 10000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(876, 698), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(871, 698), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(876, 698), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 10000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 38:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(38);
		m_workThread->PostTask(spMainTask);
	}
	case 39:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(39);
		spFightTask->setParam({ 1741, 286 }, 15000, 45, 1, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 40:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(40);
		m_workThread->PostTask(spMainTask);
	}
	case 41:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(41);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1701, 75), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1114, 290), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(664, 346), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(635, 409), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(722, 456), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(906, 547), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1376, 238), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 3000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 42:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(42);
		m_workThread->PostTask(spMainTask);
	}
	case 43:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(43);
		spFightTask->setParam({ 1741, 286 }, 50000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 44:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(44);
		m_workThread->PostTask(spMainTask);
	}
	case 45:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(45);
		spFightTask->setParam({ 1741, 286 }, 10000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 46:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(46);
		m_workThread->PostTask(spMainTask);
	}
	case 47:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(47);
		spFightTask->setParam({ 1741, 286 }, 10000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 48:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(48);
		m_workThread->PostTask(spMainTask);
	}
	case 49:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(49);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(883, 492), 20000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(965, 614), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 10000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 50:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(50);
		m_workThread->PostTask(spMainTask);
	}
	case 51:
	{
		std::shared_ptr<TuLangTask> spTuLangTask(new TuLangTask);
		spTuLangTask->setStep(51);
		m_workThread->PostTask(spTuLangTask);
	}
	case 52:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(52);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(947, 548), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 35000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 53:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(53);
		m_workThread->PostTask(spMainTask);
	}
	case 54:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(54);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1892, 909), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(975, 778), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(752, 349), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1097, 470), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(906, 549), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1224, 280), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1423, 468), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 55:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(55);
		m_workThread->PostTask(spMainTask);
	}
	case 56:
	{
		std::shared_ptr<WaiZuiTask> spWaiZuiTask(new WaiZuiTask);
		spWaiZuiTask->setStep(56);
		m_workThread->PostTask(spWaiZuiTask);
	}
	case 57:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(57);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(947, 548), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 35000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 58:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(58);
		m_workThread->PostTask(spMainTask);
	}
	case 59:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(59);
		spFightTask->setParam({ 1741, 286 }, 2000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 60:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(60);
		m_workThread->PostTask(spMainTask);
	}
	case 61:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(61);
		spFightTask->setParam({ 1741, 286 }, 70000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 62:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(62);
		m_workThread->PostTask(spMainTask);
	}
	case 63:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(63);
		spFightTask->setParam({ 1741, 286 }, 20000, 5, 5, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 64:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(64);
		m_workThread->PostTask(spMainTask);
	}
	case 65:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(65);
		spFightTask->setParam({ 1765, 318 }, 3000, 0, 0, { 1759, 318 });
		m_workThread->PostTask(spFightTask);
	}
	case 66:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(66);
		m_workThread->PostTask(spMainTask);
	}
	case 67:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(67);
		spFightTask->setParam({ 1741, 286 }, 65000, 0, 0, { 1759, 302 });
		m_workThread->PostTask(spFightTask);
	}
	case 68:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(68);
		m_workThread->PostTask(spMainTask);
	}
	case 69:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(69);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(891, 693), 20000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(953, 740), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(971, 627), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1065, 896), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 896), 1500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1065, 896), 1500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 896), 1500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1065, 896), 1500));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1032, 336), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1760, 336), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1754, 383), 55000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1751, 430), 35000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1756, 479), 60000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1748, 525), 70000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(866, 515), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 464), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(909, 378), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(866, 515), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 464), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(909, 378), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(866, 515), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 464), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(909, 378), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(866, 515), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 464), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(909, 378), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(866, 515), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1070, 464), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(909, 378), 5000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1713, 286), 4000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(943, 565), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1713, 286), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1713, 286), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1713, 286), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1713, 286), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(950, 547), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1741, 286), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(879, 692), 40000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(955, 743), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(970, 625), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1303, 301), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 25000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 70:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(70);
		m_workThread->PostTask(spMainTask);
	}
	case 71:
	{
		std::shared_ptr<FengMingShanTask> spFengMingShanTask(new FengMingShanTask);
		spFengMingShanTask->setStep(71);
		m_workThread->PostTask(spFengMingShanTask);
	}
	case 72:
	{
		std::shared_ptr<ClickTask> spClickTask(new ClickTask);
		spClickTask->setStep(72);
		std::vector<std::pair<xyls::Point, int32_t>> vecClick;
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(947, 548), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(1759, 302), 1000));
		vecClick.push_back(std::pair<xyls::Point, int32_t>(xyls::Point(785, 613), 35000));
		spClickTask->setParam(m_checkWorkThreadId, vecClick);
		m_workThread->PostTask(spClickTask);
	}
	case 73:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(73);
		m_workThread->PostTask(spMainTask);
	}
	case 74:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(74);
		spFightTask->setParam({ 1760, 319 }, 5000, 0, 0, { 1760, 319 });
		m_workThread->PostTask(spFightTask);
	}
	case 75:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(75);
		m_workThread->PostTask(spMainTask);
	}
	case 76:
	{
		std::shared_ptr<FightTask> spFightTask(new FightTask);
		spFightTask->setStep(76);
		spFightTask->setParam({ 1741, 286 }, 6000, 0, 0, { 1746, 286 });
		m_workThread->PostTask(spFightTask);
	}
	case 77:
	{
		std::shared_ptr<MainTask> spMainTask(new MainTask);
		spMainTask->setStep(77);
		m_workThread->PostTask(spMainTask);
	}
	default:
		break;
	}
	
}