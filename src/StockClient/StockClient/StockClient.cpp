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

StockClient::StockClient(QWidget* parent)
	: QMainWindow(parent),
	m_threadCount(0),
	m_sendTaskThreadId(0),
	m_stockCount(0),
	m_everyTestButton(nullptr),
	m_openTonghuashunButton(nullptr),
	m_saveAllStockButton(nullptr)
{
	ui.setupUi(this);
	m_stockEdit = new LineEdit(this);
	m_everyTestButton = new COriginalButton(this);
	m_openTonghuashunButton = new COriginalButton(this);
	m_saveAllStockButton = new COriginalButton(this);
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

	m_everyTestButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_everyTestButton->setText(QStringLiteral("单元测试"));
	QObject::connect(m_everyTestButton, &COriginalButton::clicked, this, &StockClient::onEveryTestButtonClicked);
	
	m_openTonghuashunButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_openTonghuashunButton->setText(QStringLiteral("打开tonghuashun"));
	QObject::connect(m_openTonghuashunButton, &COriginalButton::clicked, this, &StockClient::onOpenTonghuashunButtonClicked);

	m_saveAllStockButton->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	m_saveAllStockButton->setText(QStringLiteral("保存所有gupiao"));
	QObject::connect(m_saveAllStockButton, &COriginalButton::clicked, this, &StockClient::onSaveAllStockButtonClicked);

	QObject::connect(&StockClientLogicManager::instance(), &StockClientLogicManager::taskTip, this, &StockClient::onTaskTip, Qt::QueuedConnection);

	m_stockEdit->setText("1,3507");
	m_stockEdit->setVisible(false);

	m_sendTaskThreadId = CTaskThreadManager::Instance().Init();
	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_threadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

bool StockClient::check()
{
	return m_everyTestButton != nullptr &&
		m_stockEdit != nullptr;
}

void StockClient::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_stockEdit->setGeometry(100, 30, 100, 30);
	m_openTonghuashunButton->setGeometry(0, 0, 100, 30);
	m_everyTestButton->setGeometry(360, 120, 160, 80);
	m_saveAllStockButton->setGeometry(110, 0, 100, 30);
}

void StockClient::closeEvent(QCloseEvent* eve)
{
	QMainWindow::closeEvent(eve);
	setVisible(false);
#ifdef _DEBUG
	CSystem::killProcess(CSystem::processPid(L"StockClientd")[0]);
#else
	CSystem::killProcess(CSystem::processPid(L"StockClient")[0]);
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

void StockClient::onTaskTip()
{
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("所有任务发送完毕");
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}