#include "Quant.h"
#include "QuantParam.h"
#include "CSystem/CSystemAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "ClientReceive.h"
#include "QtControls/COriginalButton.h"
#include "Config.h"
#include "DialogManager/DialogManagerAPI.h"
#include "StockCharge/StockChargeAPI.h"

Quant::Quant(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_button = new COriginalButton(this);
	m_calc = new COriginalButton(this);
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

	m_clientReceive = new ClientReceive;
	NetSender::instance().initClientReceive(m_clientReceive);
	ProcessWork::instance().initReceive();
	NetSender::instance().initClient(PROJECT_QUANT, std::string("QuantServer") + QUANT_SERVER_VERSION, g_config.m_computerName);

	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(100, 0, 0, 255));
	setPalette(pattle);

	m_button->setText(QStringLiteral("send_test"));
	m_button->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_button, &COriginalButton::clicked, this, &Quant::onButtonClicked);

	m_calc->setText(QStringLiteral("calc"));
	m_calc->setBkgColor(QColor(255, 0, 0, 255), QColor(0, 255, 0, 255), QColor(0, 0, 255, 255), QColor(255, 0, 0, 255));
	QObject::connect(m_calc, &COriginalButton::clicked, this, &Quant::onCalcClicked);
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

	std::vector<COriginalButton*> vecButton;
	vecButton.push_back(m_button);
	vecButton.push_back(m_calc);

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

void Quant::closeEvent(QCloseEvent* eve)
{
	ProcessWork::instance().uninitReceive();
	delete m_clientReceive;
	m_clientReceive = nullptr;
}

void Quant::onButtonClicked()
{
	NetSender::instance().sendServer(PROJECT_QUANT, std::to_string(QUANT_PRINT));
}

void Quant::onCalcClicked()
{
	BigNumber all;
	{
		BigNumber sell = 6.08;
		BigNumber buy = 6.0575;
		BigNumber money = 0;
		BigNumber position = 1000;
		int day = 200;
		while (day-- != 0)
		{
			if (day % 10 == 0 || day % 10 == 1 || day % 10 == 5 || day % 10 == 6)
			{
				sell = 6.08;
			}
			else if (day % 10 == 2)
			{
				sell = 6.08;
			}
			else if (day % 10 == 7)
			{
				sell = 6.08;
			}
			else
			{
				sell = 6.08;
			}
			auto sellFee = StockCharge::instance().sellFee("002567", sell, position * 100);
			money = money + sell * position * 100 - sellFee;
			position = (money / (buy * 100)).toPrec(0, BigNumber::ROUND_OFF);
			auto buyFee = StockCharge::instance().buyFee("002567", buy, position * 100);
			money = money - buy * position * 100 - buyFee;
		}
		all = ((((all + money + buy * position * 100) / (buy * 100000)).toPrec(4) - 1) * 100).toPrec(2);
	}
	TipDialogParam tipDialogParam;
	tipDialogParam.m_tip = QString::fromStdString(all.toString());
	tipDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(tipDialogParam);
}