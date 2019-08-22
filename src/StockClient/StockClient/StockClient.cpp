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
#include "SaveAllMarketTask.h"
#include "CheckAllStockTask.h"
#include "SaveMarketToMysqlTask.h"
#include "SaveIndicatorToMysqlTask.h"
#include "InitRedisTask.h"

StockClient::StockClient(QWidget* parent)
	: QMainWindow(parent),
	m_threadCount(0),
	m_sendTaskThreadId(0),
	m_stockCount(0),
	m_everyTestButton(nullptr),
	m_openTonghuashunButton(nullptr),
	m_saveAllStockButton(nullptr),
	m_saveAllMarketButton(nullptr),
	m_checkAllMarketButton(nullptr),
	m_saveMarketToMysqlButton(nullptr),
	m_saveIndicatorToMysqlButton(nullptr),
	m_initRedisButton(nullptr)
{
	ui.setupUi(this);
	m_everyTestButton = new COriginalButton(this);
	m_openTonghuashunButton = new COriginalButton(this);
	m_saveAllStockButton = new COriginalButton(this);
	m_saveAllMarketButton = new COriginalButton(this);
	m_checkAllMarketButton = new COriginalButton(this);
	m_saveMarketToMysqlButton = new COriginalButton(this);
	m_saveIndicatorToMysqlButton = new COriginalButton(this);
	m_initRedisButton = new COriginalButton(this);
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
	m_threadCount = CSystem::GetCPUCoreCount() * 2;
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);

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

	m_saveAllMarketButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveAllMarketButton->setText(QStringLiteral("保存所有hangqing"));
	QObject::connect(m_saveAllMarketButton, &COriginalButton::clicked, this, &StockClient::onSaveAllMarketButtonClicked);

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

	m_sendTaskThreadId = CTaskThreadManager::Instance().Init();
	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_threadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

bool StockClient::check()
{
	return m_everyTestButton != nullptr;
}

void StockClient::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_openTonghuashunButton->setGeometry(0, 0, 100, 30);
	m_everyTestButton->setGeometry(360, 120, 160, 80);
	m_saveAllStockButton->setGeometry(110, 0, 100, 30);
	m_saveAllMarketButton->setGeometry(220, 0, 100, 30);
	m_checkAllMarketButton->setGeometry(330, 0, 100, 30);
	m_saveMarketToMysqlButton->setGeometry(440, 0, 100, 30);
	m_saveIndicatorToMysqlButton->setGeometry(0, 40, 100, 30);
	m_initRedisButton->setGeometry(110, 40, 100, 30);
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
	std::shared_ptr<EveryTestSendTask> spEveryTestSendTask(new EveryTestSendTask);
	spEveryTestSendTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spEveryTestSendTask);
}

void StockClient::onOpenTonghuashunButtonClicked()
{
	StockClientLogicManager::instance().openTonghuashun();
}

void StockClient::onSaveAllStockButtonClicked()
{
	std::shared_ptr<SaveAllStockTask> spSaveAllStockTask(new SaveAllStockTask);
	spSaveAllStockTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllStockTask);
}

void StockClient::onSaveAllMarketButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_defaultText = QStringLiteral("1");
	inputDialogParam.m_editTip = QStringLiteral("请输入从第几个开始");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	std::shared_ptr<SaveAllMarketTask> spSaveAllMarketTask(new SaveAllMarketTask);
	spSaveAllMarketTask->setParam(atoi(inputDialogParam.m_editText.toStdString().c_str()) - 1, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveAllMarketTask);
}

void StockClient::onCheckAllMarketButtonClicked()
{
	std::shared_ptr<CheckAllStockTask> spCheckAllStockTask(new CheckAllStockTask);
	spCheckAllStockTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spCheckAllStockTask);
}

void StockClient::onSaveMarketToMysqlButtonClicked()
{
	std::shared_ptr<SaveMarketToMysqlTask> spSaveMarketToMysqlTask(new SaveMarketToMysqlTask);
	spSaveMarketToMysqlTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveMarketToMysqlTask);
}

void StockClient::onSaveIndicatorToMysqlButtonClicked()
{
	std::shared_ptr<SaveIndicatorToMysqlTask> spSaveIndicatorToMysqlTask(new SaveIndicatorToMysqlTask);
	spSaveIndicatorToMysqlTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spSaveIndicatorToMysqlTask);
}

void StockClient::onInitRedisButtonClicked()
{
	std::shared_ptr<InitRedisTask> spInitRedisTaskTask(new InitRedisTask);
	spInitRedisTaskTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_sendTaskThreadId)->PostTask(spInitRedisTaskTask);
}

void StockClient::onTaskTip(const QString tip)
{
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = tip;
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
	RCSend("error stock = %s", CStringManager::UnicodeToAnsi(tip.toStdWString()).c_str());
}