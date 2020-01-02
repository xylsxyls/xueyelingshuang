#include "StockClient.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/LineEdit.h"
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
#include "StockStrategy/StockStrategyAPI.h"
#include "ChooseStockTask.h"
#include "GetFilterStockTask.h"
#include "UpdateTodayToMemoryTask.h"
#include "UpdateTodayRedisTask.h"
#include "SaveFilterStockTaskToMysql.h"
#include "SaveAllFilterStockTaskToRedis.h"
#include "RealTestTask.h"
#include "OnceTestTask.h"
#include "StockSolution/StockSolutionAPI.h"
#include "EverydaySolutionTask.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "MinimizeTask.h"

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
	m_everydaySolutionButton(nullptr),
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
	m_everydaySolutionButton = new COriginalButton(this);
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
	m_everyTestButton->setText(QStringLiteral("��Ԫ����"));
	QObject::connect(m_everyTestButton, &COriginalButton::clicked, this, &StockClient::onEveryTestButtonClicked);
	
	m_openTonghuashunButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_openTonghuashunButton->setText(QStringLiteral("��tonghuashun"));
	QObject::connect(m_openTonghuashunButton, &COriginalButton::clicked, this, &StockClient::onOpenTonghuashunButtonClicked);

	m_saveAllStockButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveAllStockButton->setText(QStringLiteral("��������gupiao"));
	QObject::connect(m_saveAllStockButton, &COriginalButton::clicked, this, &StockClient::onSaveAllStockButtonClicked);

	m_saveGroupMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveGroupMarketButton->setText(QStringLiteral("��������hangqing"));
	QObject::connect(m_saveGroupMarketButton, &COriginalButton::clicked, this, &StockClient::onSaveGroupMarketButtonClicked);

	m_win7SaveGroupMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_win7SaveGroupMarketButton->setText(QStringLiteral("win7��������hangqing"));
	QObject::connect(m_win7SaveGroupMarketButton, &COriginalButton::clicked, this, &StockClient::onWin7SaveGroupMarketButtonClicked);

	m_checkAllMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_checkAllMarketButton->setText(QStringLiteral("�������hangqing"));
	QObject::connect(m_checkAllMarketButton, &COriginalButton::clicked, this, &StockClient::onCheckAllMarketButtonClicked);

	m_saveMarketToMysqlButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveMarketToMysqlButton->setText(QStringLiteral("hangqing���"));
	QObject::connect(m_saveMarketToMysqlButton, &COriginalButton::clicked, this, &StockClient::onSaveMarketToMysqlButtonClicked);

	m_saveIndicatorToMysqlButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveIndicatorToMysqlButton->setText(QStringLiteral("����zhibiao�����"));
	QObject::connect(m_saveIndicatorToMysqlButton, &COriginalButton::clicked, this, &StockClient::onSaveIndicatorToMysqlButtonClicked);

	m_initRedisButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_initRedisButton->setText(QStringLiteral("��ʼ��redis"));
	QObject::connect(m_initRedisButton, &COriginalButton::clicked, this, &StockClient::onInitRedisButtonClicked);

	m_addAvgButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_addAvgButton->setText(QStringLiteral("����avg"));
	QObject::connect(m_addAvgButton, &COriginalButton::clicked, this, &StockClient::onAddAvgButtonClicked);

	m_mairubishengButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_mairubishengButton->setText(QStringLiteral("mairubisheng"));
	QObject::connect(m_mairubishengButton, &COriginalButton::clicked, this, &StockClient::onMairubishengButtonClicked);

	m_showAvgButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_showAvgButton->setText(QStringLiteral("��ʾavg"));
	QObject::connect(m_showAvgButton, &COriginalButton::clicked, this, &StockClient::onShowAvgButtonClicked);

	m_updateTodayMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_updateTodayMarketButton->setText(QStringLiteral("���µ���hangqing"));
	QObject::connect(m_updateTodayMarketButton, &COriginalButton::clicked, this, &StockClient::onUpdateTodayMarketButtonClicked);

	m_updateTodayIndicatorButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_updateTodayIndicatorButton->setText(QStringLiteral("���µ���zhibiao"));
	QObject::connect(m_updateTodayIndicatorButton, &COriginalButton::clicked, this, &StockClient::onUpdateTodayIndicatorButtonClicked);

	m_updateTodayRedisButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_updateTodayRedisButton->setText(QStringLiteral("���µ���redis"));
	QObject::connect(m_updateTodayRedisButton, &COriginalButton::clicked, this, &StockClient::onUpdateTodayRedisButtonClicked);

	m_chooseStockButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_chooseStockButton->setText(QStringLiteral("ÿ��xuangu"));
	QObject::connect(m_chooseStockButton, &COriginalButton::clicked, this, &StockClient::onChooseStockButtonClicked);

	m_saveFilterStockToMysqlButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveFilterStockToMysqlButton->setText(QStringLiteral("�������gupiao��mysql"));
	QObject::connect(m_saveFilterStockToMysqlButton, &COriginalButton::clicked, this, &StockClient::onSaveFilterStockToMysqlButtonClicked);

	m_saveFilterStockToRedisButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveFilterStockToRedisButton->setText(QStringLiteral("�������gupiao��redis"));
	QObject::connect(m_saveFilterStockToRedisButton, &COriginalButton::clicked, this, &StockClient::onSaveFilterStockToRedisButtonClicked);

	m_realTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_realTestButton->setText(QStringLiteral("ģ��ʵ�ʲ���"));
	QObject::connect(m_realTestButton, &COriginalButton::clicked, this, &StockClient::onRealTestButtonClicked);

	m_onceTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_onceTestButton->setText(QStringLiteral("����ģ�����"));
	QObject::connect(m_onceTestButton, &COriginalButton::clicked, this, &StockClient::onOnceTestButtonClicked);

	m_everydaySolutionButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everydaySolutionButton->setText(QStringLiteral("ÿ�շ���"));
	QObject::connect(m_everydaySolutionButton, &COriginalButton::clicked, this, &StockClient::onEverydaySolutionButtonClicked);

	m_everydayTaskButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everydayTaskButton->setText(QStringLiteral("ÿ������"));
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
	inputDialogParam.m_editTip = QStringLiteral("���������룺");
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
	vecButton.push_back(m_everydaySolutionButton);
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
#ifdef _DEBUG
	CSystem::killProcess(CSystem::processPid(L"StockClientd.exe")[0]);
#else
	CSystem::killProcess(CSystem::processPid(L"StockClient.exe")[0]);
#endif
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
	line.m_tip = QStringLiteral("��ʼ");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_defaultText = QStringLiteral("0");
	line.m_tip = QStringLiteral("����");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("�����뿪ʼ�ͽ�������ţ�����ͷβ");
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
	line.m_tip = QStringLiteral("��ʼ");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_defaultText = QStringLiteral("0");
	line.m_tip = QStringLiteral("����");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("�����뿪ʼ�ͽ�������ţ�����ͷβ");
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
	inputDialogParam.m_editTip = QStringLiteral("��Ҫ�������1");
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
	spGetAllFilterStockTask->setParam((HWND)winId(), IntDateTime(0, 0), true, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

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
	line.m_tip = QStringLiteral("����");
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
	line.m_tip = QStringLiteral("���");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("���");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("������һ���avg���ݣ�");
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
	inputDialogParam.m_editTip = QStringLiteral("��������ʱ�䣨���룩");
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
	line.m_tip = QStringLiteral("��ʼ����");
	line.m_defaultText = QStringLiteral("2019-06-21");
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("��������");
	line.m_defaultText = QStringLiteral("2019-09-16");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("��������Ҫչʾ��avg���ݣ�");
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
	line.m_tip = QStringLiteral("��ѯ����");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("�ػ�hangqing����");
	line.m_defaultText = QStringLiteral("0");
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("��������Ҫѡ�������");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	RCSend("time = %d", ::GetTickCount());

	std::shared_ptr<OpenProcessTask> spOpenMessageTestTask(new OpenProcessTask);
	spOpenMessageTestTask->setParam(CSystem::commonFile("MessageTest"));
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenMessageTestTask);

	std::shared_ptr<OpenProcessTask> spOpenProcessTask(new OpenProcessTask);
	spOpenProcessTask->setParam(StockClientLogicManager::instance().tonghuashunPath(), 1000, 8000);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOpenProcessTask);

	if (inputDialogParam.m_vecInputEx[1].m_editText.toStdString() == "1")
	{
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

	std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
	std::vector<StrategyType> vecStrategyType;
	vecStrategyType.push_back(SAR_RISE_BACK);
	vecStrategyType.push_back(SAR_RISE_BACK);
	spChooseStockTask->setParam(inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str(),
		std::vector<std::string>(),
		vecStrategyType,
		AVG_FUND_HIGH_SCORE,
		nullptr,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);
}

void StockClient::onSaveFilterStockToMysqlButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("��ʼ����");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("��������");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("��������Ҫѡ�������");
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
			std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
			spGetAllFilterStockTask->setParam((HWND)winId(), currentTime, false, this);
			CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

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
	line.m_tip = QStringLiteral("��ʼ����");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("��������");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("��������Ҫѡ�������");
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
	line.m_tip = QStringLiteral("��ʼ����");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("��������");
	line.m_defaultText = "2019-11-15";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("��������Ҫѡ�������");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();

	std::shared_ptr<RealTestTask> spRealTestTask(new RealTestTask);
	std::vector<StrategyType> vecStrategyType;
	vecStrategyType.push_back(SAR_RISE_BACK);
	vecStrategyType.push_back(SAR_RISE_BACK);
	spRealTestTask->setParam(AVG_FUND_HIGH_SCORE, vecStrategyType, beginTime, endTime, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spRealTestTask);
}

void StockClient::onOnceTestButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("��ʼ����");
	line.m_defaultText = "2019-10-08";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("��������");
	line.m_defaultText = "2019-11-15";
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("��������Ҫѡ�������");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	IntDateTime beginTime = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	IntDateTime endTime = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();

	std::shared_ptr<OnceTestTask> spOnceTestTask(new OnceTestTask);
	std::vector<StrategyType> vecStrategyType;
	vecStrategyType.push_back(SAR_RISE_BACK);
	vecStrategyType.push_back(SAR_RISE_BACK);
	spOnceTestTask->setParam(DISPOSABLE_STRATEGY, vecStrategyType, beginTime, endTime, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spOnceTestTask);
}

void StockClient::onEverydaySolutionButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_editTip = QStringLiteral("�Ƿ����»�ȡ�����ļ�");
	inputDialogParam.m_defaultText = QStringLiteral("1");
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

	std::shared_ptr<SaveGroupMarketTask> spSaveGroupMarketTask(new SaveGroupMarketTask);
	spSaveGroupMarketTask->setParam("", this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveGroupMarketTask);

	std::shared_ptr<UpdateTodayToMemoryTask> spUpdateTodayToMemoryTask(new UpdateTodayToMemoryTask);
	spUpdateTodayToMemoryTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spUpdateTodayToMemoryTask);

	std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	spGetAllFilterStockTask->setParam((HWND)winId(), IntDateTime(0, 0), inputDialogParam.m_editText == "1", this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

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

	std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
	std::vector<StrategyType> vecStrategyType;
	vecStrategyType.push_back(SAR_RISE_BACK);
	vecStrategyType.push_back(SAR_RISE_BACK);
	spChooseStockTask->setParam(IntDateTime(0, 0),
		std::vector<std::string>(),
		vecStrategyType,
		AVG_FUND_HIGH_SCORE,
		&m_stockFund,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);

	std::shared_ptr<EverydaySolutionTask> spEverydaySolutionTask(new EverydaySolutionTask);
	spEverydaySolutionTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spEverydaySolutionTask);
}

void StockClient::onEverydayTaskButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_editTip = QStringLiteral("�Ƿ����»�ȡ�����ļ�");
	inputDialogParam.m_defaultText = QStringLiteral("1");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	RCSend("time = %d", ::GetTickCount());

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

	std::shared_ptr<GetFilterStockTask> spGetAllFilterStockTask(new GetFilterStockTask);
	spGetAllFilterStockTask->setParam((HWND)winId(), IntDateTime(0, 0), inputDialogParam.m_editText == "1", this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spGetAllFilterStockTask);

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
	
	std::shared_ptr<ChooseStockTask> spChooseStockTask(new ChooseStockTask);
	std::vector<StrategyType> vecStrategyType;
	vecStrategyType.push_back(SAR_RISE_BACK);
	vecStrategyType.push_back(SAR_RISE_BACK);
	spChooseStockTask->setParam(IntDateTime(0, 0),
		std::vector<std::string>(),
		vecStrategyType,
		AVG_FUND_HIGH_SCORE,
		nullptr,
		this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spChooseStockTask);

	//std::shared_ptr<MairubishengTask> spMairubishengTask(new MairubishengTask);
	//spMairubishengTask->setParam(atoi(inputDialogParam.m_editText.toStdString().c_str()), this);
	//CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spMairubishengTask);
}

void StockClient::onTaskTip(const QString tip)
{
	RCSend("time = %d, tip = %s", (int32_t)::GetTickCount(), CStringManager::UnicodeToAnsi(tip.toStdWString()).c_str());
	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = tip;
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}