#include "SolutionWidget.h"
#include "QtControls/Label.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include "DialogManager/DialogManagerAPI.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "StockDraw.h"

SolutionWidget::SolutionWidget(QWidget* widget):
m_time(nullptr),
m_size(nullptr),
m_hasBuy(nullptr)
{
	setParent(widget);
	init();
}

void SolutionWidget::init()
{
	QObject::connect(this, &SolutionWidget::solutionSignal, this, &SolutionWidget::onSolutionSignal, Qt::QueuedConnection);
	setGeometry(660, 721, 600, 319);
	setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);

	m_exit->show();
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "res/close.png");
	QObject::connect(m_exit, &COriginalButton::clicked, this, &SolutionWidget::reject);

	m_time = new Label(this);
	m_size = new Label(this);
	m_hasBuy = new COriginalButton(this);
	m_hasBuy->setBkgColor(QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255));
	QObject::connect(m_hasBuy, &COriginalButton::clicked, this, &SolutionWidget::onHasBuyButtonClicked);

	int32_t index = -1;
	while (index++ != 5 - 1)
	{
		Label* buy = new Label(this);
		buy->setBackgroundColor(QColor(255, 0, 0, 255));
		m_vecBuy.push_back(buy);
		Label* sell = new Label(this);
		sell->setBackgroundColor(QColor(0, 255, 0, 255));
		m_vecSell.push_back(sell);
		COriginalButton* buyCancel = new COriginalButton(this);
		buyCancel->setBkgColor(QColor(255, 255, 0, 255), QColor(255, 255, 0, 255), QColor(255, 255, 0, 255), QColor(255, 255, 0, 255));
		QObject::connect(buyCancel, &COriginalButton::clicked, this, &SolutionWidget::onBuyCancelButtonClicked);
		m_vecBuyCancel.push_back(buyCancel);
	}
}

void SolutionWidget::resizeEvent(QResizeEvent* eve)
{
	DialogShow::resizeEvent(eve);

	int32_t widgetWidth = width();
	int32_t widgetHeight = height();
	m_exit->setGeometry(widgetWidth - 26 - 5, 5, 26, 26);

	m_time->setGeometry(20, 10, 200, 30);
	m_size->setGeometry(280, 10, 200, 30);

	int32_t index = -1;
	while (index++ != 5 - 1)
	{
		m_vecBuy[index]->setGeometry(280, 50 + index * 35, 200, 30);
		m_vecSell[index]->setGeometry(30, 50 + index * 35, 200, 30);
		m_vecBuyCancel[index]->setGeometry(500, 50 + index * 35, 80, 30);
	}
}

void SolutionWidget::onSolutionSignal(SolutionWidgetParam solutionWidgetParam)
{
	show();

	m_solutionWidgetParam = solutionWidgetParam;

	m_time->setText(m_solutionWidgetParam.m_date.dateToString().c_str());
	m_size->setText(std::to_string(m_solutionWidgetParam.m_buyStock.size()).c_str());

	int32_t index = -1;
	while (index++ != 5 - 1)
	{
		m_vecBuy[index]->setText("");
		m_vecSell[index]->setText("");
	}

	index = -1;
	while (index++ != m_solutionWidgetParam.m_sellStock.size() - 1)
	{
		if (index == 5)
		{
			break;
		}
		m_vecSell[index]->setText((m_solutionWidgetParam.m_sellStock[index].first +
			m_solutionWidgetParam.m_sellStock[index].second.first.toString() +
			m_solutionWidgetParam.m_sellStock[index].second.second.toString()).c_str());
	}

	index = -1;
	while (index++ != m_solutionWidgetParam.m_buyStock.size() - 1)
	{
		if (index == 5)
		{
			break;
		}
		m_vecBuy[index]->setText((m_solutionWidgetParam.m_buyStock[index].first +
			m_solutionWidgetParam.m_buyStock[index].second.first.toString() +
			m_solutionWidgetParam.m_buyStock[index].second.second.toString()).c_str());
	}
}

void SolutionWidget::onBuyCancelButtonClicked()
{
	COriginalButton* buyCancel = qobject_cast<COriginalButton*>(sender());
	int32_t index = -1;
	while (index++ != m_vecBuyCancel.size() - 1)
	{
		if (m_vecBuyCancel[index] == buyCancel)
		{
			if ((int32_t)m_solutionWidgetParam.m_buyStock.size() > index)
			{
				m_solutionWidgetParam.m_buyStock.erase(m_solutionWidgetParam.m_buyStock.begin() + index);
			}
			break;
		}
	}
	onSolutionSignal(m_solutionWidgetParam);
}

void SolutionWidget::onHasBuyButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("zijin");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("jiage");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("cangwei");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("riqi");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("gupiao");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("«Î ‰»Î¿˙ ∑£∫");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	BigNumber allFund = inputDialogParam.m_vecInputEx[0].m_editText.toStdString().c_str();
	BigNumber price = inputDialogParam.m_vecInputEx[1].m_editText.toStdString().c_str();
	BigNumber position = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	IntDateTime date = inputDialogParam.m_vecInputEx[3].m_editText.toStdString();
	std::string stock = inputDialogParam.m_vecInputEx[4].m_editText.toStdString();
	BigNumber rate = price * (position + 30) / allFund.toPrec(6);
	GLOBAL_CONFIG[TRADE_FUND] = allFund.toString();
	int32_t size = GLOBAL_CONFIG[TRADE_NOTE_SIZE].toInt32();
	GLOBAL_CONFIG[TRADE_NOTE_SIZE] = size + 1;
	GLOBAL_CONFIG[TRADE_NOTE][size] = price.toString() + "," + rate.toString() + "," + date.toString() + "," + stock;
}