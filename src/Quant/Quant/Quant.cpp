#include "Quant.h"
#include "QuantParam.h"
#include "CSystem/CSystemAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "ClientReceive.h"
#include "QtControls/COriginalButton.h"
//#include "11Controls/controls/DialogManager.h"

Quant::Quant(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	init();
}

Quant::~Quant()
{

}

void Quant::init()
{
	if (!check())
	{
		return;
	}

	std::string computerName = CSystem::getComputerName();
	m_clientReceive = new ClientReceive;
	NetSender::instance().initClientReceive(m_clientReceive);
	ProcessWork::instance().initReceive();
	NetSender::instance().initClient(PROJECT_DTWS, std::string("QuantServer") + QUANT_SERVER_VERSION, computerName);

	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &Quant::onButtonClicked);
}

bool Quant::check()
{
	return true;
	return m_button != nullptr;
}

void Quant::resizeEvent(QResizeEvent* eve)
{
	QMainWindow::resizeEvent(eve);

	if (!check())
	{
		return;
	}
	m_button->setGeometry(360, 120, 160, 80);
}

void Quant::onButtonClicked()
{
	NetSender::instance().sendServer(PROJECT_QUANT, std::to_string(QUANT_PRINT));
}