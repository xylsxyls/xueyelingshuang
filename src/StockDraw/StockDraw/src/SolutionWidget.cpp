#include "SolutionWidget.h"
#include "QtControls/Label.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include "DialogManager/DialogManagerAPI.h"
#include "ConfigManager/ConfigManagerAPI.h"
#include "StockDraw.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMysql/StockMysqlAPI.h"

SolutionWidget::SolutionWidget(QWidget* widget):
m_time(nullptr),
m_size(nullptr),
m_hasBuy(nullptr),
m_sell(nullptr),
m_allFund(nullptr),
m_default(nullptr),
m_filterSize(nullptr),
m_default1(nullptr),
m_default2(nullptr)
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
	m_hasBuy->setText(QStringLiteral("mairu"));
	QObject::connect(m_hasBuy, &COriginalButton::clicked, this, &SolutionWidget::onHasBuyButtonClicked);

	m_sell = new COriginalButton(this);
	m_sell->setBkgColor(QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255));
	m_sell->setText(QStringLiteral("maichu"));
	QObject::connect(m_sell, &COriginalButton::clicked, this, &SolutionWidget::onSellButtonClicked);

	m_allFund = new COriginalButton(this);
	m_allFund->setBkgColor(QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255));
	m_allFund->setText(QStringLiteral("zijin"));
	QObject::connect(m_allFund, &COriginalButton::clicked, this, &SolutionWidget::onAllFundButtonClicked);

	m_default = new COriginalButton(this);
	m_default->setBkgColor(QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255), QColor(255, 0, 255, 255));
	m_default->setText(QStringLiteral("moren"));
	QObject::connect(m_default, &COriginalButton::clicked, this, &SolutionWidget::onDefaultButtonClicked);

	m_filterSize = new Label(this);
	m_default1 = new Label(this);
	m_default2 = new Label(this);

	int32_t index = -1;
	while (index++ != 5 - 1)
	{
		Label* buy = new Label(this);
		buy->setTextColor(QColor(255, 0, 0, 255));
		m_vecBuy.push_back(buy);
		Label* sell = new Label(this);
		sell->setTextColor(QColor(0, 255, 0, 255));
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

	m_allFund->setGeometry(30, 60, 40, 30);
	m_sell->setGeometry(80, 60, 40, 30);
	m_hasBuy->setGeometry(130, 60, 40, 30);
	m_default->setGeometry(180, 60, 40, 30);

	m_filterSize->setGeometry(230, 60, 30, 30);
	m_default1->setGeometry(270, 40, 300, 30);
	m_default2->setGeometry(270, 80, 300, 30);

	int32_t index = -1;
	while (index++ != 5 - 1)
	{
		m_vecBuy[index]->setGeometry(280, 120 + index * 35, 200, 30);
		m_vecSell[index]->setGeometry(30, 120 + index * 35, 200, 30);
		m_vecBuyCancel[index]->setGeometry(500, 120 + index * 35, 80, 30);
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

	std::map<std::string, std::string> mapName;
	std::vector<std::vector<std::string>> vecName = StockMysql::instance().stockNameDb();
	index = -1;
	while (index++ != vecName.size() - 1)
	{
		mapName[vecName[index][0]] = mapName[vecName[index][1]];
	}

	index = -1;
	while (index++ != m_solutionWidgetParam.m_sellStock.size() - 1)
	{
		if (index == 5)
		{
			break;
		}
		const std::string& stock = m_solutionWidgetParam.m_sellStock[index].first;
		m_vecSell[index]->setText((stock + " " +
			mapName[stock] +
			m_solutionWidgetParam.m_sellStock[index].second.first.toString() + " " +
			m_solutionWidgetParam.m_sellStock[index].second.second.toPrec(2).toString()).c_str());
	}

	if (m_solutionWidgetParam.m_buyStock.empty())
	{
		return;
	}

	BigNumber allPersent = min((int32_t)m_solutionWidgetParam.m_buyStock.size(), 5);
	index = -1;
	while (index++ != m_solutionWidgetParam.m_buyStock.size() - 1)
	{
		if (index == 5)
		{
			break;
		}
		const std::string& stock = m_solutionWidgetParam.m_buyStock[index].first;
		m_vecBuy[index]->setText((stock + " " +
			mapName[stock] +
			m_solutionWidgetParam.m_buyStock[index].second.first.toString() + " " + 
			(1 / allPersent.toPrec(6)).toPrec(2).toString()).c_str());
		allPersent = allPersent - 1;
	}

	std::string defaultStock1 = GLOBAL_CONFIG[DEFAULT_STOCK_1].toString();
	std::string defaultStock2 = GLOBAL_CONFIG[DEFAULT_STOCK_2].toString();

	StockMarket market1;
	market1.loadFromRedis(defaultStock1);
	market1.load();
	if (!market1.setDate(m_solutionWidgetParam.m_date))
	{
		return;
	}
	std::shared_ptr<StockDay> spDay1 = market1.day();
	BigNumber open1 = spDay1->open();
	BigNumber high1 = spDay1->high();
	BigNumber low1 = spDay1->low();
	BigNumber close1 = spDay1->close();

	StockIndicator::instance().loadIndicatorFromRedis(defaultStock1);
	std::shared_ptr<StockBollIndicator> spIndicator1 = StockIndicator::instance().boll();
	spIndicator1->load();
	std::shared_ptr<StockBoll> spBoll1 = spIndicator1->day(m_solutionWidgetParam.m_date);
	BigNumber bollUp1 = spBoll1->m_up;
	BigNumber bollMid1 = spBoll1->m_mid;
	BigNumber bollDown1 = spBoll1->m_down;

	m_default1->setText((open1.toString() + " " + 
		high1.toString() + " " +
		low1.toString() + " " +
		close1.toString() + " " +
		bollUp1.toString() + " " +
		bollMid1.toString() + " " +
		bollDown1.toString()).c_str());

	StockMarket market2;
	market2.loadFromRedis(defaultStock2);
	market2.load();
	if (!market2.setDate(m_solutionWidgetParam.m_date))
	{
		return;
	}
	std::shared_ptr<StockDay> spDay2 = market2.day();
	BigNumber open2 = spDay2->open();
	BigNumber high2 = spDay2->high();
	BigNumber low2 = spDay2->low();
	BigNumber close2 = spDay2->close();

	StockIndicator::instance().loadIndicatorFromRedis(defaultStock2);
	std::shared_ptr<StockBollIndicator> spIndicator2 = StockIndicator::instance().boll();
	spIndicator2->load();
	std::shared_ptr<StockBoll> spBoll2 = spIndicator2->day(m_solutionWidgetParam.m_date);
	BigNumber bollUp2 = spBoll2->m_up;
	BigNumber bollMid2 = spBoll2->m_mid;
	BigNumber bollDown2 = spBoll2->m_down;

	m_default2->setText((open2.toString() + " " +
		high2.toString() + " " +
		low2.toString() + " " +
		close2.toString() + " " +
		bollUp2.toString() + " " +
		bollMid2.toString() + " " +
		bollDown2.toString()).c_str());

	std::vector<std::string> vecFilterStock;
	StockMysql::instance().readFilterStockFromRedis(m_solutionWidgetParam.m_date, vecFilterStock);

	int32_t vecFilterStockSize = vecFilterStock.size();

	m_filterSize->setText(std::to_string(vecFilterStockSize).c_str());
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
	line.m_tip = QStringLiteral("gupiao");
	line.m_defaultText = "";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("jiage");
	line.m_defaultText = "";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("cangwei");
	line.m_defaultText = "";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("riqi");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入历史：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	std::string stock = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	BigNumber price = inputDialogParam.m_vecInputEx[1].m_editText.toStdString().c_str();
	BigNumber position = inputDialogParam.m_vecInputEx[2].m_editText.toStdString().c_str();
	IntDateTime date = inputDialogParam.m_vecInputEx[3].m_editText.toStdString();
	BigNumber rate = price * (position + 30) / BigNumber(GLOBAL_CONFIG[TRADE_FUND].toString().c_str()).toPrec(6);
	std::string configTradeNote = GLOBAL_CONFIG[TRADE_NOTE].toString();
	if (!configTradeNote.empty())
	{
		configTradeNote.push_back('|');
	}
	GLOBAL_CONFIG[TRADE_NOTE] = configTradeNote + stock + "," + price.toString() + "," + rate.toString() + "," + date.dateToString();

	StockMarket market;
	market.loadFromRedis(stock);
	market.load();
	if (!market.setDate(date))
	{
		return;
	}
	m_solutionWidgetParam.m_stockFund->buyStock(price, rate, market.day());
}

void SolutionWidget::onSellButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("gupiao");
	line.m_defaultText = "";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("jiage");
	line.m_defaultText = "";
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("riqi");
	line.m_defaultText = IntDateTime().dateToString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入需要maichu的gupiao：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	std::string stock = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	BigNumber price = inputDialogParam.m_vecInputEx[1].m_editText.toStdString().c_str();
	IntDateTime date = inputDialogParam.m_vecInputEx[2].m_editText.toStdString();
	BigNumber rate = 1;
	std::string configTradeNote = GLOBAL_CONFIG[TRADE_NOTE].toString();
	if (configTradeNote.empty())
	{
		return;
	}
	std::vector<std::string> vecTradeNote = CStringManager::split(configTradeNote, "|");
	for (auto itTradeNote = vecTradeNote.begin(); itTradeNote != vecTradeNote.end();)
	{
		if (CStringManager::split(*itTradeNote, ",")[0] == stock)
		{
			itTradeNote = vecTradeNote.erase(itTradeNote);
			continue;
		}
		++itTradeNote;
	}
	configTradeNote.clear();
	int32_t index = -1;
	while (index++ != vecTradeNote.size() - 1)
	{
		configTradeNote.append(vecTradeNote[index] + "|");
	}
	if (!configTradeNote.empty())
	{
		configTradeNote.pop_back();
	}
	GLOBAL_CONFIG[TRADE_NOTE] = configTradeNote;
	StockMarket market;
	market.loadFromRedis(stock);
	market.load();
	if (!market.setDate(date))
	{
		return;
	}
	m_solutionWidgetParam.m_stockFund->sellStock(price, rate, market.day());
}

void SolutionWidget::onAllFundButtonClicked()
{
	InputDialogParam inputDialogParam;
	inputDialogParam.m_editTip = QStringLiteral("请输入总zijin");
	inputDialogParam.m_defaultText = GLOBAL_CONFIG[TRADE_FUND].toString().c_str();
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}
	GLOBAL_CONFIG[TRADE_FUND] = inputDialogParam.m_editText.toStdString();
}

void SolutionWidget::onDefaultButtonClicked()
{
	InputDialogParam inputDialogParam;
	InputEx line;
	line.m_tip = QStringLiteral("moren1");
	line.m_defaultText = GLOBAL_CONFIG[DEFAULT_STOCK_1].toString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	line.m_tip = QStringLiteral("moren2");
	line.m_defaultText = GLOBAL_CONFIG[DEFAULT_STOCK_2].toString().c_str();
	inputDialogParam.m_vecInputEx.push_back(line);
	inputDialogParam.m_editTip = QStringLiteral("请输入打印的gupiao：");
	inputDialogParam.m_parent = windowHandle();
	DialogManager::instance().makeDialog(inputDialogParam);
	if (inputDialogParam.m_result != ACCEPT_BUTTON)
	{
		return;
	}

	GLOBAL_CONFIG[DEFAULT_STOCK_1] = inputDialogParam.m_vecInputEx[0].m_editText.toStdString();
	GLOBAL_CONFIG[DEFAULT_STOCK_2] = inputDialogParam.m_vecInputEx[1].m_editText.toStdString();

	onSolutionSignal(m_solutionWidgetParam);
}