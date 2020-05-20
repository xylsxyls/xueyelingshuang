#include "StockClient.h"
#include "QtControls/COriginalButton.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "EveryTestTask.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DialogManager/DialogManagerAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "EveryTestSendTask.h"
#include "StockClientLogicManager.h"
#include "SaveAllStockTask.h"
#include "SaveGroupMarketTask.h"
#include "CheckAllStockTask.h"
#include "SaveMarketToMysqlTask.h"
#include "SaveIndicatorToMysqlTask.h"
#include "InitRedisTask.h"
#include "Win7SaveGroupMarketTask.h"
#include "SaveTodayMarketFileTask.h"
#include "TodayMarketFileToMemoryTask.h"
#include "MairubishengTask.h"
#include "OpenProcessTask.h"
#include "UpdateTodayMarketTask.h"
#include "UpdateTodayIndicatorTask.h"
#include "StockDraw/StockDrawAPI.h"
#include "StockParam.h"
#include "ChooseStockTask.h"
#include "GetFilterStockTask.h"
#include "UpdateTodayToMemoryTask.h"
#include "UpdateTodayRedisTask.h"
#include "SaveFilterStockTaskToMysql.h"
#include "SaveAllFilterStockTaskToRedis.h"
#include "RealTestTask.h"
#include "OnceTestTask.h"
#include "DaysTestTask.h"
#include "RankTestTask.h"
#include "ChanceTestTask.h"
#include "EverydaySolutionTask.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "MinimizeTask.h"
#include "StockParam.h"
#include "SyntheticalTestTask.h"
#include "TipTask.h"

StockClient::StockClient(QWidget* parent)
	: QMainWindow(parent),
	m_threadCount(0),
	m_sendTaskThreadId(0),
	m_stockCount(0),
	m_everyTestButton(nullptr),
	m_openTonghuashunButton(nullptr),
	m_saveAllStockButton(nullptr),
	m_saveGroupMarketButton(nullptr),
	m_checkAllMarketButton(nullptr),
	m_saveMarketToMysqlButton(nullptr),
	m_saveIndicatorToMysqlButton(nullptr),
	m_initRedisButton(nullptr),
	m_addAvgButton(nullptr),
	m_mairubishengButton(nullptr),
	m_showAvgButton(nullptr),
	m_updateTodayMarketButton(nullptr),
	m_today(0, 0),
	m_updateTodayIndicatorButton(nullptr),
	m_updateTodayRedisButton(nullptr),
	m_chooseStockButton(nullptr),
	m_saveFilterStockToMysqlButton(nullptr),
	m_saveFilterStockToRedisButton(nullptr),
	m_realTestButton(nullptr),
	m_onceTestButton(nullptr),
	m_daysTestButton(nullptr),
	m_rankTestButton(nullptr),
	m_chanceTestButton(nullptr),
	m_syntheticalTestButton(nullptr),
	m_arithmeticsTestButton(nullptr),
	m_everydaySolutionButton(nullptr),
	m_everydayHelperButton(nullptr),
	m_everydayTaskButton(nullptr)
{
	ui.setupUi(this);
	m_everyTestButton = new COriginalButton(this);
	m_openTonghuashunButton = new COriginalButton(this);
	m_saveAllStockButton = new COriginalButton(this);
	m_win7SaveGroupMarketButton = new COriginalButton(this);
	m_saveGroupMarketButton = new COriginalButton(this);
	m_checkAllMarketButton = new COriginalButton(this);
	m_saveMarketToMysqlButton = new COriginalButton(this);
	m_saveIndicatorToMysqlButton = new COriginalButton(this);
	m_initRedisButton = new COriginalButton(this);
	m_addAvgButton = new COriginalButton(this);
	m_mairubishengButton = new COriginalButton(this);
	m_showAvgButton = new COriginalButton(this);
	m_updateTodayMarketButton = new COriginalButton(this);
	m_updateTodayIndicatorButton = new COriginalButton(this);
	m_updateTodayRedisButton = new COriginalButton(this);
	m_chooseStockButton = new COriginalButton(this);
	m_saveFilterStockToMysqlButton = new COriginalButton(this);
	m_saveFilterStockToRedisButton = new COriginalButton(this);
	m_realTestButton = new COriginalButton(this);
	m_onceTestButton = new COriginalButton(this);
	m_daysTestButton = new COriginalButton(this);
	m_rankTestButton = new COriginalButton(this);
	m_chanceTestButton = new COriginalButton(this);
	m_syntheticalTestButton = new COriginalButton(this);
	m_arithmeticsTestButton = new COriginalButton(this);
	m_everydaySolutionButton = new COriginalButton(this);
	m_everydayHelperButton = new COriginalButton(this);
	m_everydayTaskButton = new COriginalButton(this);
	init();
}

StockClient::~StockClient()
{

}

void StockClient::init()
{
	if (!check())
	{
		return;
	}
	
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);

	StockDraw::instance();

	m_threadCount = CSystem::GetCPUCoreCount() * 2;
	m_stockFund.add(GLOBAL_CONFIG[TRADE_FUND].toString().c_str());
	
	std::string configTradeNote = GLOBAL_CONFIG[TRADE_NOTE].toString();
	std::vector<std::string> vecTradeNote = CStringManager::split(configTradeNote, "|");
	int32_t index = -1;
	while (!configTradeNote.empty() && index++ != vecTradeNote.size() - 1)
	{
		std::vector<std::string> vecTradeParam = CStringManager::split(vecTradeNote[index], ",");
		StockMarket market;
		market.loadFromRedis(vecTradeParam[0]);
		market.load();
		market.setDate(vecTradeParam[3]);
		m_stockFund.buyStock(vecTradeParam[1].c_str(), vecTradeParam[2].c_str(), market.day());
	}

	QObject::connect(&StockClientLogicManager::instance(), &StockClientLogicManager::taskTip, this, &StockClient::onTaskTip, Qt::QueuedConnection);

	m_everyTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everyTestButton->setText(QStringLiteral("单元测试"));
	QObject::connect(m_everyTestButton, &COriginalButton::clicked, this, &StockClient::onEveryTestButtonClicked);
	
	m_openTonghuashunButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_openTonghuashunButton->setText(QStringLiteral("打开tonghuashun"));
	QObject::connect(m_openTonghuashunButton, &COriginalButton::clicked, this, &StockClient::onOpenTonghuashunButtonClicked);

	m_saveAllStockButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveAllStockButton->setText(QStringLiteral("保存所有gupiao"));
	QObject::connect(m_saveAllStockButton, &COriginalButton::clicked, this, &StockClient::onSaveAllStockButtonClicked);

	m_saveGroupMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveGroupMarketButton->setText(QStringLiteral("保存所有hangqing"));
	QObject::connect(m_saveGroupMarketButton, &COriginalButton::clicked, this, &StockClient::onSaveGroupMarketButtonClicked);

	m_win7SaveGroupMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_win7SaveGroupMarketButton->setText(QStringLiteral("win7保存所有hangqing"));
	QObject::connect(m_win7SaveGroupMarketButton, &COriginalButton::clicked, this, &StockClient::onWin7SaveGroupMarketButtonClicked);

	m_checkAllMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_checkAllMarketButton->setText(QStringLiteral("检测所有hangqing"));
	QObject::connect(m_checkAllMarketButton, &COriginalButton::clicked, this, &StockClient::onCheckAllMarketButtonClicked);

	m_saveMarketToMysqlButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveMarketToMysqlButton->setText(QStringLiteral("hangqing入库"));
	QObject::connect(m_saveMarketToMysqlButton, &COriginalButton::clicked, this, &StockClient::onSaveMarketToMysqlButtonClicked);

	m_saveIndicatorToMysqlButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveIndicatorToMysqlButton->setText(QStringLiteral("计算zhibiao并入库"));
	QObject::connect(m_saveIndicatorToMysqlButton, &COriginalButton::clicked, this, &StockClient::onSaveIndicatorToMysqlButtonClicked);

	m_initRedisButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_initRedisButton->setText(QStringLiteral("初始化redis"));
	QObject::connect(m_initRedisButton, &COriginalButton::clicked, this, &StockClient::onInitRedisButtonClicked);

	m_addAvgButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_addAvgButton->setText(QStringLiteral("增加avg"));
	QObject::connect(m_addAvgButton, &COriginalButton::clicked, this, &StockClient::onAddAvgButtonClicked);

	m_mairubishengButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_mairubishengButton->setText(QStringLiteral("mairubisheng"));
	QObject::connect(m_mairubishengButton, &COriginalButton::clicked, this, &StockClient::onMairubishengButtonClicked);

	m_showAvgButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_showAvgButton->setText(QStringLiteral("显示avg"));
	QObject::connect(m_showAvgButton, &COriginalButton::clicked, this, &StockClient::onShowAvgButtonClicked);

	m_updateTodayMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_updateTodayMarketButton->setText(QStringLiteral("更新当天hangqing"));
	QObject::connect(m_updateTodayMarketButton, &COriginalButton::clicked, this, &StockClient::onUpdateTodayMarketButtonClicked);

	m_updateTodayIndicatorButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_updateTodayIndicatorButton->setText(QStringLiteral("更新当天zhibiao"));
	QObject::connect(m_updateTodayIndicatorButton, &COriginalButton::clicked, this, &StockClient::onUpdateTodayIndicatorButtonClicked);

	m_updateTodayRedisButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_updateTodayRedisButton->setText(QStringLiteral("更新当天redis"));
	QObject::connect(m_updateTodayRedisButton, &COriginalButton::clicked, this, &StockClient::onUpdateTodayRedisButtonClicked);

	m_chooseStockButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_chooseStockButton->setText(QStringLiteral("每日xuangu"));
	QObject::connect(m_chooseStockButton, &COriginalButton::clicked, this, &StockClient::onChooseStockButtonClicked);

	m_saveFilterStockToMysqlButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveFilterStockToMysqlButton->setText(QStringLiteral("存入过滤gupiao到mysql"));
	QObject::connect(m_saveFilterStockToMysqlButton, &COriginalButton::clicked, this, &StockClient::onSaveFilterStockToMysqlButtonClicked);

	m_saveFilterStockToRedisButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveFilterStockToRedisButton->setText(QStringLiteral("存入过滤gupiao到redis"));
	QObject::connect(m_saveFilterStockToRedisButton, &COriginalButton::clicked, this, &StockClient::onSaveFilterStockToRedisButtonClicked);

	m_realTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_realTestButton->setText(QStringLiteral("模拟实际测试"));
	QObject::connect(m_realTestButton, &COriginalButton::clicked, this, &StockClient::onRealTestButtonClicked);

	m_onceTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_onceTestButton->setText(QStringLiteral("单次模拟测试"));
	QObject::connect(m_onceTestButton, &COriginalButton::clicked, this, &StockClient::onOnceTestButtonClicked);

	m_daysTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_daysTestButton->setText(QStringLiteral("天数模拟测试"));
	QObject::connect(m_daysTestButton, &COriginalButton::clicked, this, &StockClient::onDaysTestButtonClicked);

	m_rankTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_rankTestButton->setText(QStringLiteral("排名模拟测试"));
	QObject::connect(m_rankTestButton, &COriginalButton::clicked, this, &StockClient::onRankTestButtonClicked);

	m_chanceTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_chanceTestButton->setText(QStringLiteral("几率模拟测试"));
	QObject::connect(m_chanceTestButton, &COriginalButton::clicked, this, &StockClient::onChanceTestButtonClicked);

	m_syntheticalTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_syntheticalTestButton->setText(QStringLiteral("综合模拟测试"));
	QObject::connect(m_syntheticalTestButton, &COriginalButton::clicked, this, &StockClient::onSyntheticalTestButtonClicked);

	m_arithmeticsTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_arithmeticsTestButton->setText(QStringLiteral("运算测试"));
	QObject::connect(m_arithmeticsTestButton, &COriginalButton::clicked, this, &StockClient::onArithmeticsTestButtonClicked);

	m_everydaySolutionButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everydaySolutionButton->setText(QStringLiteral("每日方案"));
	QObject::connect(m_everydaySolutionButton, &COriginalButton::clicked, this, &StockClient::onEverydaySolutionButtonClicked);

	m_everydayHelperButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everydayHelperButton->setText(QStringLiteral("每日帮助"));
	QObject::connect(m_everydayHelperButton, &COriginalButton::clicked, this, &StockClient::onEverydayHelperButtonClicked);

	m_everydayTaskButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everydayTaskButton->setText(QStringLiteral("每日任务"));
	QObject::connect(m_everydayTaskButton, &COriginalButton::clicked, this, &StockClient::onEverydayTaskButtonClicked);

	m_sendTaskThreadId = CTaskThreadManager::Instance().Init();
	index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_threadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

bool StockClient::check()
{
	return m_everyTestButton != nullptr;
}

bool StockClient::askPassword()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_title = QStringLiteral("StockClient");
	inputDialogParam.m_editTip = QStringLiteral("请输入密码：");
	inputDialogParam.m_isPassword = true;
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != DialogResult::ACCEPT_BUTTON)
	{
		return false;
	}
	if (inputDialogParam.m_editText != PASSWORD)
	{
		return false;
	}
	return true;
}

void StockClient::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	std::vector<COriginalButton*> vecButton;
	vecButton.push_back(m_openTonghuashunButton);
	vecButton.push_back(m_saveAllStockButton);
	vecButton.push_back(m_win7SaveGroupMarketButton);
	vecButton.push_back(m_saveGroupMarketButton);
	vecButton.push_back(m_checkAllMarketButton);
	vecButton.push_back(m_saveMarketToMysqlButton);
	vecButton.push_back(m_saveIndicatorToMysqlButton);
	vecButton.push_back(m_initRedisButton);
	vecButton.push_back(m_everyTestButton);
	vecButton.push_back(m_addAvgButton);
	vecButton.push_back(m_mairubishengButton);
	vecButton.push_back(m_showAvgButton);
	vecButton.push_back(m_updateTodayMarketButton);
	vecButton.push_back(m_updateTodayIndicatorButton);
	vecButton.push_back(m_updateTodayRedisButton);
	vecButton.push_back(m_chooseStockButton);
	vecButton.push_back(m_saveFilterStockToMysqlButton);
	vecButton.push_back(m_saveFilterStockToRedisButton);
	vecButton.push_back(m_realTestButton);
	vecButton.push_back(m_onceTestButton);
	vecButton.push_back(m_daysTestButton);
	vecButton.push_back(m_rankTestButton);
	vecButton.push_back(m_chanceTestButton);
	vecButton.push_back(m_syntheticalTestButton);
	vecButton.push_back(m_arithmeticsTestButton);
	vecButton.push_back(m_everydaySolutionButton);
	vecButton.push_back(m_everydayHelperButton);
	vecButton.push_back(m_everydayTaskButton);

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

void StockClient::closeEvent(QCloseEvent* eve)
{
	QMainWindow::closeEvent(eve);
	setVisible(false);
	CTaskThreadManager::Instance().UninitAll();
	CSystem::killProcess(CSystem::processFirstPid(CSystem::GetCurrentExeName() + ".exe"));
}

void StockClient::onEveryTestButtonClicked()
{
	RCSend("time = %d", ::GetTickCount());

	//std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	//spGetAllFilterStockTask->setParam((HWND)winId(), "2019-10-09", this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

	std::shared_ptr<EveryTestTask> spEveryTestTask(new EveryTestTask);
	spEveryTestTask->setParam(SAR_RISE_BACK, "2019-08-01", "2019-09-30", this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spEveryTestTask);
}

void StockClient::onOpenTonghuashunButtonClicked()
{
	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);
}

void StockClient::onSaveAllStockButtonClicked()
{
	if (!askPassword())
	{
		return;
	}

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
	spSaveTodayMarketFileTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	std::shared_ptr<SaveAllStockTask> spSaveAllStockTask(new SaveAllStockTask);
	spSaveAllStockTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllStockTask);
}

void StockClient::onWin7SaveGroupMarketButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_defaultText = QStringLiteral("1");
	line.m_tip = QStringLiteral("开始");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_defaultText = QStringLiteral("0");
	line.m_tip = QStringLiteral("结束");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入开始和结束的序号，包括头尾");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != DialogResult::ACCEPT_BUTTON)
	{
		return;
	}

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<Win7SaveGroupMarketTask> spWin7SaveGroupMarketTask(new Win7SaveGroupMarketTask);
	spWin7SaveGroupMarketTask->setParam(atoi(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str()) - 1,
		atoi(inputDialogParam.m_vecInputEx[1].m_editText.toStdString().c_str()),
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spWin7SaveGroupMarketTask);
}

void StockClient::onSaveGroupMarketButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_defaultText = QStringLiteral("1");
	line.m_tip = QStringLiteral("开始");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_defaultText = QStringLiteral("0");
	line.m_tip = QStringLiteral("结束");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入开始和结束的序号，包括头尾");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != DialogResult::ACCEPT_BUTTON)
	{
		return;
	}

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
	spSaveGroupMarketTask->setParam(atoi(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str()) - 1,
		atoi(inputDialogParam.m_vecInputEx[1].m_editText.toStdString().c_str()),
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);
}

void StockClient::onCheckAllMarketButtonClicked()
{
	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	std::shared_ptr<CheckAllStockTask> spCheckAllStockTask(new CheckAllStockTask);
	spCheckAllStockTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spCheckAllStockTask);
}

void StockClient::onSaveMarketToMysqlButtonClicked()
{
	if (!askPassword())
	{
		return;
	}
	std::shared_ptr<SaveMarketToMysqlTask> spSaveMarketToMysqlTask(new SaveMarketToMysqlTask);
	spSaveMarketToMysqlTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveMarketToMysqlTask);
}

void StockClient::onSaveIndicatorToMysqlButtonClicked()
{
	if (!askPassword())
	{
		return;
	}
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_defaultText = QStringLiteral("1");
	line.m_tip = "wr";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = "rsi";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = "sar";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = "boll";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("需要计算的填1");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	std::shared_ptr<SaveIndicatorToMysqlTask> spSaveIndicatorToMysqlTask(new SaveIndicatorToMysqlTask);
	spSaveIndicatorToMysqlTask->setParam(atoi(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str()) == 1,
		atoi(inputDialogParam.m_vecInputEx[1].m_editText.toStdString().c_str()) == 1,
		atoi(inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str()) == 1,
		atoi(inputDialogParam.m_vecInputEx[3].m_editText.toStdString().c_str()) == 1,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveIndicatorToMysqlTask);
}

void StockClient::onUpdateTodayRedisButtonClicked()
{
	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
	spSaveTodayMarketFileTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
	spSaveGroupMarketTask->setParam("", this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);

	std::shared_ptr<UpdateTodayToMemoryTask> spUpdateTodayToMemoryTask(new UpdateTodayToMemoryTask);
	spUpdateTodayToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayToMemoryTask);

	std::shared_ptr<UpdateTodayRedisTask> spUpdateTodayRedisTask(new UpdateTodayRedisTask);
	spUpdateTodayRedisTask->setParam(StockMysql::instance().allStock(), false, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayRedisTask);

	std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	spGetAllFilterStockTask->setParam((HWND)winId(), IntDateTime(0, 0), SEARCH_STR, &m_allFilterStock, "", true, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

	std::shared_ptr<GetFilterStockTask> spGetLiftBanStockTask(new GetFilterStockTask);
	spGetLiftBanStockTask->setParam((HWND)winId(), IntDateTime(0, 0), LIFTBAN_STR, &m_liftBanStock, LIFTBAN_NAME, true, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetLiftBanStockTask);

	std::shared_ptr<SaveFilterStockTaskToMysql> spSaveAllFilterStockTaskToMysql(new SaveFilterStockTaskToMysql);
	spSaveAllFilterStockTaskToMysql->setParam(IntDateTime(0, 0), this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToMysql);

	std::shared_ptr<SaveAllFilterStockTaskToRedis> spSaveAllFilterStockTaskToRedis(new SaveAllFilterStockTaskToRedis);
	spSaveAllFilterStockTaskToRedis->setParam(&m_today, &m_today, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToRedis);
}

void StockClient::onInitRedisButtonClicked()
{
	if (!askPassword())
	{
		return;
	}
	std::shared_ptr<InitRedisTask> spInitRedisTaskTask(new InitRedisTask);
	spInitRedisTaskTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spInitRedisTaskTask);
}

void StockClient::onAddAvgButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("gupiao");
	line.m_defaultText = QStringLiteral("002912");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("日期");
	line.m_defaultText = QStringLiteral("2019-08-01");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("09:30");
	line.m_defaultText = QStringLiteral("0");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("10:00");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("10:30");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("11:00");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("11:30");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("13:30");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("14:00");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("14:30");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("15:00");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("最高");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("最低");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入一天的avg数据：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	std::string stock = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime date = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	std::map<IntDateTime, std::shared_ptr<StockAvg>> avgData;
	avgData[date] = std::shared_ptr<StockAvg>(new StockAvg);
	std::shared_ptr<StockAvg>& stockAvg = avgData.find(date)->second;
	stockAvg->m_avg09_30 = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	stockAvg->m_avg10_00 = inputDialogParam.m_vecInputEx[3].m_editText.toStdString().c_str();
	stockAvg->m_avg10_30 = inputDialogParam.m_vecInputEx[4].m_editText.toStdString().c_str();
	stockAvg->m_avg11_00 = inputDialogParam.m_vecInputEx[5].m_editText.toStdString().c_str();
	stockAvg->m_avg11_30 = inputDialogParam.m_vecInputEx[6].m_editText.toStdString().c_str();
	stockAvg->m_avg13_30 = inputDialogParam.m_vecInputEx[7].m_editText.toStdString().c_str();
	stockAvg->m_avg14_00 = inputDialogParam.m_vecInputEx[8].m_editText.toStdString().c_str();
	stockAvg->m_avg14_30 = inputDialogParam.m_vecInputEx[9].m_editText.toStdString().c_str();
	stockAvg->m_avg15_00 = inputDialogParam.m_vecInputEx[10].m_editText.toStdString().c_str();
	stockAvg->m_avgHigh = inputDialogParam.m_vecInputEx[11].m_editText.toStdString().c_str();
	stockAvg->m_avgLow = inputDialogParam.m_vecInputEx[12].m_editText.toStdString().c_str();
	StockIndicator::instance().saveAvg(stock, avgData);
}

void StockClient::onMairubishengButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_editTip = QStringLiteral("请输入间隔时间（毫秒）");
	inputDialogParam.m_defaultText = QStringLiteral("2500");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
	spSaveTodayMarketFileTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	std::shared_ptr<MairubishengTask> spMairubishengTask(new MairubishengTask);
	spMairubishengTask->setParam(atoi(inputDialogParam.m_editText.toStdString().c_str()), this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spMairubishengTask);
}

void StockClient::onShowAvgButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("gupiao");
	line.m_defaultText = QStringLiteral("002912");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = QStringLiteral("2019-06-21");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = QStringLiteral("2019-09-16");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要展示的avg数据：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	std::string stock = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime beginDate = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	IntDateTime endDate = inputDialogParam.m_vecInputEx[2].m_editText.toStdString();

	StockDraw::instance().showAvgKLine(stock, beginDate, endDate);
}

void StockClient::onUpdateTodayMarketButtonClicked()
{
	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
	spSaveTodayMarketFileTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
	spSaveGroupMarketTask->setParam("", this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);

	std::shared_ptr<UpdateTodayToMemoryTask> spUpdateTodayToMemoryTask(new UpdateTodayToMemoryTask);
	spUpdateTodayToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayToMemoryTask);

	std::shared_ptr<UpdateTodayMarketTask> spUpdateTodayMarketTask(new UpdateTodayMarketTask);
	spUpdateTodayMarketTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayMarketTask);
}

void StockClient::onUpdateTodayIndicatorButtonClicked()
{
	std::shared_ptr<UpdateTodayIndicatorTask> spUpdateTodayIndicatorTask(new UpdateTodayIndicatorTask);
	spUpdateTodayIndicatorTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayIndicatorTask);
}

void StockClient::onChooseStockButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("查询日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("重获hangqing数据");
	line.m_defaultText = QStringLiteral("0");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("2和以上");
	line.m_defaultText = "2";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	if (inputDialogParam.m_vecInputEx[1].m_editText.toStdString() == "1")
	{
		std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
		spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

		std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
		spSaveTodayMarketFileTask->setParam(this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

		std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
		spTodayMarketFileToMemoryTask->setParam(this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

		std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
		spSaveGroupMarketTask->setParam("", this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);

		std::shared_ptr<UpdateTodayToMemoryTask> spUpdateTodayToMemoryTask(new UpdateTodayToMemoryTask);
		spUpdateTodayToMemoryTask->setParam(this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayToMemoryTask);

		std::shared_ptr<UpdateTodayRedisTask> spUpdateTodayRedisTask(new UpdateTodayRedisTask);
		spUpdateTodayRedisTask->setParam(StockMysql::instance().allStock(), false, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayRedisTask);
	}

	//std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	//spGetAllFilterStockTask->setParam((HWND)winId(), inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str(), this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);
	//
	//std::shared_ptr<SaveFilterStockTaskToMysql> spSaveAllFilterStockTaskToMysql(new SaveFilterStockTaskToMysql);
	//spSaveAllFilterStockTaskToMysql->setParam(IntDateTime(0, 0), this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToMysql);
	//
	//std::shared_ptr<SaveAllFilterStockTaskToRedis> spSaveAllFilterStockTaskToRedis(new SaveAllFilterStockTaskToRedis);
	//spSaveAllFilterStockTaskToRedis->setParam(&m_today, &m_today, this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToRedis);

	std::string allStrategyType = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
	spChooseStockTask->setParam(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str(),
		std::vector<std::string>(),
		toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE),
		INTEGRATED_STRATEGY,
		nullptr,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
}

void StockClient::onSaveFilterStockToMysqlButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	if (beginTime > endTime)
	{
		return;
	}
	IntDateTime currentTime = beginTime;
	do
	{
		std::string path = CSystem::GetCurrentExePath() + currentTime.dateToString() + ".xls";
		if (CSystem::fileExist(path))
		{
			m_allFilterStock.clear();
			std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
			spGetAllFilterStockTask->setParam((HWND)winId(), currentTime, SEARCH_STR, &m_allFilterStock, "", false, this);
			CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

			m_liftBanStock.clear();
			std::shared_ptr<GetFilterStockTask> spGetLiftBanStockTask(new GetFilterStockTask);
			spGetLiftBanStockTask->setParam((HWND)winId(), currentTime, LIFTBAN_STR, &m_liftBanStock, LIFTBAN_NAME, false, this);
			CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetLiftBanStockTask);

			std::shared_ptr<SaveFilterStockTaskToMysql> spSaveAllFilterStockTaskToMysql(new SaveFilterStockTaskToMysql);
			spSaveAllFilterStockTaskToMysql->setParam(currentTime, this);
			CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToMysql);
		}
		currentTime = currentTime + 86400;
	} while (currentTime <= endTime);
}

void StockClient::onSaveFilterStockToRedisButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();

	std::shared_ptr<SaveAllFilterStockTaskToRedis> spSaveAllFilterStockTaskToRedis(new SaveAllFilterStockTaskToRedis);
	spSaveAllFilterStockTaskToRedis->setParam(new IntDateTime(beginTime), new IntDateTime(endTime), this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToRedis);
}

void StockClient::onRealTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = "2020-05-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("2和以上");
	line.m_defaultText = "2";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	std::string allStrategyType = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
	spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
}

void StockClient::onOnceTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = "2020-05-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("2和以上");
	line.m_defaultText = "2";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	std::string allStrategyType = inputDialogParam.m_vecInputEx[2].m_editText.toStdString();
	std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
	spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
}

void StockClient::onDaysTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = "2020-05-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("2和以上");
	line.m_defaultText = "2";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	std::string allStrategyType = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();

	std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
	spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
}

void StockClient::onRankTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = "2020-05-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("2和以上");
	line.m_defaultText = "2";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	std::string allStrategyType = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	
	std::shared_ptr<RankTestTask> spRankTestTask(new RankTestTask);
	spRankTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRankTestTask);
}

void StockClient::onChanceTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = "2020-05-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("2和以上");
	line.m_defaultText = "2";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("最大天数");
	line.m_defaultText = "10";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	std::string allStrategyType = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	int32_t maxDay = atoi(inputDialogParam.m_vecInputEx[3].m_editText.toStdString().c_str());

	std::shared_ptr<ChanceTestTask> spChanceTestTask(new ChanceTestTask);
	spChanceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, maxDay, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChanceTestTask);
}

void StockClient::onSyntheticalTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("开始日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("结束日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("开始数值");
	line.m_defaultText = "2905.19";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要选择参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();
	BigNumber beginValue = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
	std::vector<std::string> allStock = StockMysql::instance().allStock();
	//std::vector<std::string> allStock = StockStrategy::instance().strategyAllStock(beginTime, endTime);
	spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
}

void StockClient::onArithmeticsTestButtonClicked()
{
	std::vector<std::string> allStock = StockMysql::instance().allStock();

	tipSend("real");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "2";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real2");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real3");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "5";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real4");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3,2.1";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real5");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3,5,2.1";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real6");
	{
		IntDateTime beginTime = "2019-10-05";
		IntDateTime endTime = "2019-11-17";
		std::string allStrategyType = "2";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real7");
	{
		IntDateTime beginTime = "2019-10-05";
		IntDateTime endTime = "2019-11-17";
		std::string allStrategyType = "5";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real8");
	{
		IntDateTime beginTime = "2019-01-01";
		IntDateTime endTime = "2019-12-31";
		std::string allStrategyType = "5";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real9");
	{
		IntDateTime beginTime = "2020-04-28";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3,2.1";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("real10");
	{
		IntDateTime beginTime = "2019-10-05";
		IntDateTime endTime = "2019-11-17";
		std::string allStrategyType = "2.1";
		std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
		spRealTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
	}
	
	tipSend("once");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "2";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("once2");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("once3");
	{
		IntDateTime beginTime = "2019-10-08";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "5";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("once4");
	{
		IntDateTime beginTime = "2019-10-05";
		IntDateTime endTime = "2019-11-17";
		std::string allStrategyType = "2";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("once5");
	{
		IntDateTime beginTime = "2019-10-05";
		IntDateTime endTime = "2019-11-17";
		std::string allStrategyType = "5";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("once6");
	{
		IntDateTime beginTime = "2019-01-01";
		IntDateTime endTime = "2019-12-31";
		std::string allStrategyType = "5";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("once7");
	{
		IntDateTime beginTime = "2019-10-05";
		IntDateTime endTime = "2019-11-17";
		std::string allStrategyType = "2.1";
		std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
		spOnceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, DISPOSABLE_STRATEGY), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
	}
	
	tipSend("choose");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2021-04-16",
			std::vector<std::string>(),
			toChooseParam("2.1", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("choose2");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2021-04-17",
			std::vector<std::string>(),
			toChooseParam("2.1", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("choose3");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2020-05-01",
			std::vector<std::string>(),
			toChooseParam("2.1", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("choose4");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2019-09-27",
			std::vector<std::string>(),
			toChooseParam("2.1", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("choose5");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2020-01-02",
			std::vector<std::string>(),
			toChooseParam("2", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("choose6");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2020-01-02",
			std::vector<std::string>(),
			toChooseParam("2.1", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("choose7");
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		spChooseStockTask->setParam("2020-01-02",
			std::vector<std::string>(),
			toChooseParam("3", AVG_FUND_HIGH_SCORE),
			INTEGRATED_STRATEGY,
			nullptr,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}
	
	tipSend("day");
	{
		IntDateTime beginTime = "2020-03-28";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "2.1";
	
		std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
		spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
	}
	
	tipSend("day2");
	{
		IntDateTime beginTime = "2020-01-01";
		IntDateTime endTime = "2020-05-09";
		std::string allStrategyType = "2.1";
	
		std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
		spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
	}
	
	tipSend("day3");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-01-22";
		std::string allStrategyType = "2";
	
		std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
		spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
	}
	
	tipSend("day4");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3";
	
		std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
		spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
	}
	
	tipSend("day5");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3,2.1";
	
		std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
		spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
	}
	
	tipSend("day6");
	{
		IntDateTime beginTime = "2020-01-01";
		IntDateTime endTime = "2020-05-02";
		std::string allStrategyType = "2.1,5,3";
	
		std::shared_ptr<DaysTestTask> spDaysTestTask(new DaysTestTask);
		spDaysTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spDaysTestTask);
	}
	
	tipSend("rank");
	{
		IntDateTime beginTime = "2020-03-28";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "2.1";
	
		std::shared_ptr<RankTestTask> spRankTestTask(new RankTestTask);
		spRankTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRankTestTask);
	}
	
	tipSend("rank2");
	{
		IntDateTime beginTime = "2020-01-01";
		IntDateTime endTime = "2020-05-09";
		std::string allStrategyType = "2.1";
	
		std::shared_ptr<RankTestTask> spRankTestTask(new RankTestTask);
		spRankTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRankTestTask);
	}
	
	tipSend("rank3");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "2";
	
		std::shared_ptr<RankTestTask> spRankTestTask(new RankTestTask);
		spRankTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRankTestTask);
	}
	
	tipSend("rank4");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3";
	
		std::shared_ptr<RankTestTask> spRankTestTask(new RankTestTask);
		spRankTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRankTestTask);
	}
	
	tipSend("chance");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-03-02";
		std::string allStrategyType = "2.1";
		int32_t maxDay = 10;
	
		std::shared_ptr<ChanceTestTask> spChanceTestTask(new ChanceTestTask);
		spChanceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, maxDay, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChanceTestTask);
	}
	
	tipSend("chance2");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-03-02";
		std::string allStrategyType = "2.1";
		int32_t maxDay = 1;
	
		std::shared_ptr<ChanceTestTask> spChanceTestTask(new ChanceTestTask);
		spChanceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, maxDay, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChanceTestTask);
	}
	
	tipSend("chance3");
	{
		IntDateTime beginTime = "2020-01-01";
		IntDateTime endTime = "2020-05-09";
		std::string allStrategyType = "2.1";
		int32_t maxDay = 1;
	
		std::shared_ptr<ChanceTestTask> spChanceTestTask(new ChanceTestTask);
		spChanceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, maxDay, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChanceTestTask);
	}
	
	tipSend("chance4");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "2";
		int32_t maxDay = 1;
	
		std::shared_ptr<ChanceTestTask> spChanceTestTask(new ChanceTestTask);
		spChanceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, maxDay, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChanceTestTask);
	}
	
	tipSend("chance5");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		std::string allStrategyType = "3";
		int32_t maxDay = 5;
	
		std::shared_ptr<ChanceTestTask> spChanceTestTask(new ChanceTestTask);
		spChanceTestTask->setParam(INTEGRATED_STRATEGY, toChooseParam(allStrategyType, AVG_FUND_HIGH_SCORE), beginTime, endTime, maxDay, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChanceTestTask);
	}
	
	tipSend("synthetical");
	{
		IntDateTime beginTime = "2020-01-02";
		IntDateTime endTime = "2020-05-08";
		BigNumber beginValue = "3050.12";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}
	
	tipSend("synthetical2");
	{
		IntDateTime beginTime = "2020-01-01";
		IntDateTime endTime = "2020-05-09";
		BigNumber beginValue = "3050.12";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}

	tipSend("synthetical3");
	{
		IntDateTime beginTime = "2020-05-09";
		IntDateTime endTime = "2020-05-09";
		BigNumber beginValue = "2895.34";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}

	tipSend("synthetical4");
	{
		IntDateTime beginTime = "2020-05-02";
		IntDateTime endTime = "2020-05-02";
		BigNumber beginValue = "2860.08";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}

	tipSend("synthetical5");
	{
		IntDateTime beginTime = "2020-05-02";
		IntDateTime endTime = "2020-05-03";
		BigNumber beginValue = "2860.08";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}

	tipSend("synthetical6");
	{
		IntDateTime beginTime = "2020-05-02";
		IntDateTime endTime = "2020-05-06";
		BigNumber beginValue = "2860.08";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}

	tipSend("synthetical7");
	{
		IntDateTime beginTime = "2020-05-08";
		IntDateTime endTime = "2020-05-08";
		BigNumber beginValue = "2871.52";
		std::shared_ptr<SyntheticalTestTask> spSyntheticalTestTask(new SyntheticalTestTask);
		spSyntheticalTestTask->setParam(beginValue, beginTime, endTime, allStock, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSyntheticalTestTask);
	}

	tipSend("Arithmetics End");
}

void StockClient::onEverydaySolutionButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("是否重新获取过滤文件");
	line.m_defaultText = QStringLiteral("0");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入方案参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	RCSend("time = %d", ::GetTickCount());

	bool regainFilter = atoi(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str()) == 1;
	m_today = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 2000, 8000, xyls::Point(240, 1063));;
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
	spSaveTodayMarketFileTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	//std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
	//spSaveGroupMarketTask->setParam("", this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);

	//std::shared_ptr<UpdateTodayToMemoryTask> spUpdateTodayToMemoryTask(new UpdateTodayToMemoryTask);
	//spUpdateTodayToMemoryTask->setParam(this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayToMemoryTask);

	std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	spGetAllFilterStockTask->setParam((HWND)winId(), IntDateTime(0, 0), SEARCH_STR, &m_allFilterStock, "", regainFilter, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

	std::shared_ptr<GetFilterStockTask> spGetLiftBanStockTask(new GetFilterStockTask);
	spGetLiftBanStockTask->setParam((HWND)winId(), IntDateTime(0, 0), LIFTBAN_STR, &m_liftBanStock, LIFTBAN_NAME, regainFilter, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetLiftBanStockTask);

	std::shared_ptr<MinimizeTask> spMinimizeTask(new MinimizeTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spMinimizeTask);

	std::shared_ptr<UpdateTodayRedisTask> spUpdateTodayRedisTask(new UpdateTodayRedisTask);
	spUpdateTodayRedisTask->setParam(StockMysql::instance().allStock(), false, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayRedisTask);

	std::shared_ptr<SaveFilterStockTaskToMysql> spSaveAllFilterStockTaskToMysql(new SaveFilterStockTaskToMysql);
	spSaveAllFilterStockTaskToMysql->setParam(IntDateTime(0, 0), this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToMysql);

	std::shared_ptr<SaveAllFilterStockTaskToRedis> spSaveAllFilterStockTaskToRedis(new SaveAllFilterStockTaskToRedis);
	spSaveAllFilterStockTaskToRedis->setParam(&m_today, &m_today, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToRedis);

	int32_t index = SAR_RISE_BACK - 1;
	while (index++ != STRATEGY_TYPE_SIZE - 1)
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		std::vector<ChooseParam> vecChooseParam;
		vecChooseParam.push_back(ChooseParam());
		vecChooseParam.back().m_useType = (StrategyType)index;
		vecChooseParam.back().m_useCountType = (StrategyType)index;
		vecChooseParam.back().m_isObserve = false;
		vecChooseParam.back().m_solutionType = AVG_FUND_HIGH_SCORE;
		spChooseStockTask->setParam(IntDateTime(0, 0),
			std::vector<std::string>(),
			vecChooseParam,
			INTEGRATED_STRATEGY,
			&m_stockFund,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}

	//std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
	//std::vector<std::pair<StrategyType, StrategyType>> vecStrategyType;
	//vecStrategyType.push_back(std::pair<StrategyType, StrategyType>());
	//vecStrategyType.back().first = SAR_RISE_BACK;
	//vecStrategyType.back().second = SAR_RISE_BACK;
	//spChooseStockTask->setParam(IntDateTime(0, 0),
	//	std::vector<std::string>(),
	//	vecStrategyType,
	//	AVG_FUND_HIGH_SCORE,
	//	&m_stockFund,
	//	this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	//std::shared_ptr<ChooseStockTask> spChooseStockTask2(new ChooseStockTask);
	//vecStrategyType.clear();
	//vecStrategyType.push_back(std::pair<StrategyType, StrategyType>());
	//vecStrategyType.back().first = CATCH_UP;
	//vecStrategyType.back().second = CATCH_UP;
	//spChooseStockTask2->setParam(IntDateTime(0, 0),
	//	std::vector<std::string>(),
	//	vecStrategyType,
	//	AVG_FUND_HIGH_SCORE,
	//	&m_stockFund,
	//	this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask2);
	std::shared_ptr<ChooseStockTask> spChooseStockTask3(new ChooseStockTask);
	std::vector<ChooseParam> vecChooseParam;
	vecChooseParam.push_back(ChooseParam());
	vecChooseParam.back().m_useType = SAR_RISE_BACK;
	vecChooseParam.back().m_useCountType = SAR_RISE_BACK;
	vecChooseParam.back().m_isObserve = true;
	vecChooseParam.back().m_solutionType = AVG_FUND_HIGH_SCORE;
	spChooseStockTask3->setParam(IntDateTime(0, 0),
		std::vector<std::string>(),
		vecChooseParam,
		INTEGRATED_STRATEGY,
		&m_stockFund,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask3);

	std::shared_ptr<EverydaySolutionTask> spEverydaySolutionTask(new EverydaySolutionTask);
	spEverydaySolutionTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spEverydaySolutionTask);
}

void StockClient::onEverydayHelperButtonClicked()
{
	if (m_today.empty())
	{
		return;
	}
	std::string configTradeNote = GLOBAL_CONFIG[TRADE_NOTE].toString();
	if (configTradeNote.empty())
	{
		return;
	}
	std::string result;
	std::vector<std::string> vecTradeNote = CStringManager::split(configTradeNote, "|");
	for (auto itTradeNote = vecTradeNote.begin(); itTradeNote != vecTradeNote.end(); ++itTradeNote)
	{
		const std::vector<std::string>& vecStockInfo = CStringManager::split(*itTradeNote, ",");
		const std::string& stock = vecStockInfo[0];
		BigNumber price = vecStockInfo[1].c_str();
		StockIndicator::instance().loadIndicatorFromRedis(stock, m_today, m_today);
		std::shared_ptr<StockBollIndicator> spBollIndicator = StockIndicator::instance().boll();
		spBollIndicator->load();
		std::shared_ptr<StockBoll> spBoll = spBollIndicator->day(m_today);
		BigNumber firstPrice = (price * "1.003").toPrec(2, BigNumber::ROUND_UP);
		BigNumber secondPrice = ((spBoll->m_up - spBoll->m_mid) / "100.0" * 20 + spBoll->m_mid).toPrec(2, BigNumber::ROUND_UP);
		BigNumber thirdPrice = ((spBoll->m_up - spBoll->m_mid) / "100.0" * 80 + spBoll->m_mid).toPrec(2, BigNumber::ROUND_UP);
		result += (stock + " " + firstPrice.toString() + " " + secondPrice.toString() + " " + thirdPrice.toString() + "\n");
	}
	result.pop_back();
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = result.c_str();
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}

void StockClient::onEverydayTaskButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("是否重新获取过滤文件");
	line.m_defaultText = QStringLiteral("1");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("日期");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入方案参数：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	RCSend("time = %d", ::GetTickCount());

	bool regainFilter = atoi(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str()) == 1;
	m_today = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	std::shared_ptr<SaveTodayMarketFileTask> spSaveTodayMarketFileTask(new SaveTodayMarketFileTask);
	spSaveTodayMarketFileTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveTodayMarketFileTask);

	std::shared_ptr<TodayMarketFileToMemoryTask> spTodayMarketFileToMemoryTask(new TodayMarketFileToMemoryTask);
	spTodayMarketFileToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTodayMarketFileToMemoryTask);

	//std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
	//spSaveGroupMarketTask->setParam("", this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);
	//
	//std::shared_ptr<UpdateTodayToMemoryTask> spUpdateTodayToMemoryTask(new UpdateTodayToMemoryTask);
	//spUpdateTodayToMemoryTask->setParam(this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayToMemoryTask);

	std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	spGetAllFilterStockTask->setParam((HWND)winId(), IntDateTime(0, 0), SEARCH_STR, &m_allFilterStock, "", regainFilter, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

	std::shared_ptr<GetFilterStockTask> spGetLiftBanStockTask(new GetFilterStockTask);
	spGetLiftBanStockTask->setParam((HWND)winId(), IntDateTime(0, 0), LIFTBAN_STR, &m_liftBanStock, LIFTBAN_NAME, regainFilter, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetLiftBanStockTask);

	std::shared_ptr<MinimizeTask> spMinimizeTask(new MinimizeTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spMinimizeTask);

	std::shared_ptr<UpdateTodayMarketTask> spUpdateTodayMarketTask(new UpdateTodayMarketTask);
	spUpdateTodayMarketTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayMarketTask);

	std::shared_ptr<UpdateTodayIndicatorTask> spUpdateTodayIndicatorTask(new UpdateTodayIndicatorTask);
	spUpdateTodayIndicatorTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayIndicatorTask);

	std::shared_ptr<UpdateTodayRedisTask> spUpdateTodayRedisTask(new UpdateTodayRedisTask);
	spUpdateTodayRedisTask->setParam(StockMysql::instance().allStock(), true, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayRedisTask);

	std::shared_ptr<SaveFilterStockTaskToMysql> spSaveAllFilterStockTaskToMysql(new SaveFilterStockTaskToMysql);
	spSaveAllFilterStockTaskToMysql->setParam(IntDateTime(0, 0), this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToMysql);

	std::shared_ptr<SaveAllFilterStockTaskToRedis> spSaveAllFilterStockTaskToRedis(new SaveAllFilterStockTaskToRedis);
	spSaveAllFilterStockTaskToRedis->setParam(&m_today, &m_today, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllFilterStockTaskToRedis);
	
	int32_t index = SAR_RISE_BACK - 1;
	while (index++ != STRATEGY_TYPE_SIZE - 1)
	{
		std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
		std::vector<ChooseParam> vecChooseParam;
		vecChooseParam.push_back(ChooseParam());
		vecChooseParam.back().m_useType = (StrategyType)index;
		vecChooseParam.back().m_useCountType = (StrategyType)index;
		vecChooseParam.back().m_isObserve = false;
		vecChooseParam.back().m_solutionType = AVG_FUND_HIGH_SCORE;
		spChooseStockTask->setParam(IntDateTime(0, 0),
			std::vector<std::string>(),
			vecChooseParam,
			INTEGRATED_STRATEGY,
			&m_stockFund,
			this);
		CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
	}

	std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
	std::vector<ChooseParam> vecChooseParam;
	vecChooseParam.push_back(ChooseParam());
	vecChooseParam.back().m_useType = SAR_RISE_BACK;
	vecChooseParam.back().m_useCountType = SAR_RISE_BACK;
	vecChooseParam.back().m_isObserve = true;
	vecChooseParam.back().m_solutionType = AVG_FUND_HIGH_SCORE;
	spChooseStockTask->setParam(IntDateTime(0, 0),
		std::vector<std::string>(),
		vecChooseParam,
		INTEGRATED_STRATEGY,
		&m_stockFund,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);

	//std::shared_ptr<MairubishengTask> spMairubishengTask(new MairubishengTask);
	//spMairubishengTask->setParam(atoi(inputDialogParam.m_editText.toStdString().c_str()), this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spMairubishengTask);
}

void StockClient::tipSend(const std::string& tip)
{
	std::shared_ptr<TipTask> spTipTask(new TipTask);
	spTipTask->setParam(tip);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spTipTask);
}

std::vector<ChooseParam> StockClient::toChooseParam(const std::string& allStrategyType, SolutionType solutionType)
{
	std::vector<ChooseParam> vecChooseParam;
	std::vector<std::string> vecAllStrategyType = CStringManager::split(allStrategyType, ",");
	if (vecAllStrategyType.empty())
	{
		return vecChooseParam;
	}
	
	int32_t index = -1;
	while (index++ != vecAllStrategyType.size() - 1)
	{
		const std::string& everyStrategyType = vecAllStrategyType[index];
		std::vector<std::string> vecStrategyType = CStringManager::split(everyStrategyType, ".");
		if (vecStrategyType.size() > 2 || vecStrategyType.size() == 0)
		{
			return std::vector<ChooseParam>();
		}
		StrategyType strategyType = (StrategyType)atoi(vecStrategyType[0].c_str());
		bool observe = false;
		if (vecStrategyType.size() == 2)
		{
			observe = (bool)atoi(vecStrategyType[1].c_str());
		}
		vecChooseParam.push_back(ChooseParam());
		vecChooseParam.back().m_useType = strategyType;
		vecChooseParam.back().m_useCountType = strategyType;
		vecChooseParam.back().m_isObserve = observe;
		vecChooseParam.back().m_solutionType = solutionType;
	}
	return vecChooseParam;
}

void StockClient::onTaskTip(const QString tip)
{
	RCSend("tip = %s", CStringManager::UnicodeToAnsi(tip.toStdWString()).c_str());
	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = tip;
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}