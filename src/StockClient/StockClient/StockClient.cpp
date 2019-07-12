#include "StockClient.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/LineEdit.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "EveryTestTask.h"
//#include "11Controls/controls/DialogManager.h"

StockClient::StockClient(QWidget* parent)
	: QMainWindow(parent)
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
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	m_everyTest->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_everyTest, &COriginalButton::clicked, this, &StockClient::onEveryTestClicked);

	m_threadId.push_back(CTaskThreadManager::Instance().Init());
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
	std::string stock = m_stockEdit->text().toStdString();
	std::shared_ptr<EveryTestTask> spEveryTestTask(new EveryTestTask);
	spEveryTestTask->setParam(stock);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId[0])->PostTask(spEveryTestTask);

	//TipDialogParam tipDialogParam;
	//tipDialogParam.m_tip = QStringLiteral("µã»÷");
	//tipDialogParam.m_parent = windowHandle();
	//DialogManager::instance().makeDialog(tipDialogParam);
}