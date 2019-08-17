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

StockClient::StockClient(QWidget* parent)
	: QMainWindow(parent),
	m_threadCount(0)
{
	ui.setupUi(this);
	m_stockEdit = new LineEdit(this);
	m_everyTest = new COriginalButton(this);
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
	m_everyTest->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_everyTest, &COriginalButton::clicked, this, &StockClient::onEveryTestClicked);

	m_stockEdit->setText("1,3507");

	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_threadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

bool StockClient::check()
{
	return m_everyTest != nullptr &&
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
	m_everyTest->setGeometry(360, 120, 160, 80);
}

void StockClient::onEveryTestClicked()
{
	std::string beginEnd = m_stockEdit->text().toStdString();
	std::vector<std::string> vecStock = StockMysql::instance().allStock();
	std::vector<std::string> vecBeginEnd = CStringManager::split(beginEnd, ",");

	IntDateTime beginTime = "2014-02-27";
	IntDateTime endTime = "2019-02-27";

	int32_t threadIndex = -1;
	int32_t index = atoi(vecBeginEnd[0].c_str()) - 2;
	while (index++ != atoi(vecBeginEnd[1].c_str()) - 1)
	{
		//RCSend("index = %d", index + 1);
		std::shared_ptr<EveryTestTask> spEveryTestTask(new EveryTestTask);
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromDb(vecStock[index], beginTime, endTime);
		StockIndicator::instance().loadFromRedis(vecStock[index], beginTime, endTime);
		std::shared_ptr<StockWrIndicator> spStockWrIndicator = StockIndicator::instance().wr();
		std::shared_ptr<StockRsiIndicator> spStockRsiIndicator = StockIndicator::instance().rsi();
		spEveryTestTask->setParam(vecStock[index], spMarket, spStockWrIndicator, spStockRsiIndicator);
		CTaskThreadManager::Instance().GetThreadInterface(m_threadId[(++threadIndex) % m_threadCount])->PostTask(spEveryTestTask);
	}
	
	
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("所有任务发送完毕");
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}