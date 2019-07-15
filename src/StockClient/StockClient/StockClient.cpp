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
	m_spStockMysql = StockMysql::newCase();
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
	m_threadCount = 32;// CSystem::GetCPUCoreCount();
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
	std::vector<std::string> vecStock = m_spStockMysql->allStock();
	std::vector<std::string> vecBeginEnd = CStringManager::split(beginEnd, ",");

	IntDateTime beginTime = "2014-02-27";
	IntDateTime endTime = "2019-02-27";

	//std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::pair<std::shared_ptr<StockWrIndicator>, std::shared_ptr<StockRsiIndicator>>>> allData;
	//
	//RCSend("%d", ::GetTickCount());
	//int32_t index = atoi(vecBeginEnd[0].c_str()) - 2;
	//while (index++ != atoi(vecBeginEnd[1].c_str()) - 1)
	//{
	//	std::shared_ptr<StockMarket> spMarket(new StockMarket);
	//	spMarket->load(vecStock[index], beginTime, endTime);
	//	std::shared_ptr<StockWrIndicator> spStockWrIndicator(new StockWrIndicator);
	//	spStockWrIndicator->load(vecStock[index], beginTime, endTime);
	//	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator(new StockRsiIndicator);
	//	spStockRsiIndicator->load(vecStock[index], beginTime, endTime);
	//	std::pair<std::shared_ptr<StockMarket>, std::pair<std::shared_ptr<StockWrIndicator>, std::shared_ptr<StockRsiIndicator>>> pair;
	//	std::pair<std::shared_ptr<StockWrIndicator>, std::shared_ptr<StockRsiIndicator>> indipair;
	//	indipair.first = spStockWrIndicator;
	//	indipair.second = spStockRsiIndicator;
	//	pair.first = spMarket;
	//	pair.second = indipair;
	//	RCSend("index = %d", index + 1);
	//
	//	allData[vecStock[index]] = pair;
	//}
	//RCSend("%d", ::GetTickCount());
	//RCSend("load end");
	
	int32_t threadIndex = -1;
	int32_t index = atoi(vecBeginEnd[0].c_str()) - 2;
	while (index++ != atoi(vecBeginEnd[1].c_str()) - 1)
	{
		//RCSend("index = %d", index + 1);
		std::shared_ptr<EveryTestTask> spEveryTestTask(new EveryTestTask);
		//std::shared_ptr<StockMarket> spMarket(new StockMarket(*(allData.find(vecStock[index])->second.first.get())));
		//spMarket->setFirstDate();
		//spEveryTestTask->setParam(vecStock[index], spMarket, allData.find(vecStock[index])->second.second.first, allData.find(vecStock[index])->second.second.second);
		spEveryTestTask->setParam(vecStock[index]);
		CTaskThreadManager::Instance().GetThreadInterface(m_threadId[(++threadIndex) % m_threadCount])->PostTask(spEveryTestTask);
	}
	
	
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QStringLiteral("所有任务发送完毕");
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}