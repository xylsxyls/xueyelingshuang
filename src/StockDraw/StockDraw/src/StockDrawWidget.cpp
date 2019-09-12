#include "StockDrawWidget.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include <stdint.h>
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include <QPainter>

StockDrawWidget::StockDrawWidget()
{
	init();
}

void StockDrawWidget::init()
{
	QPalette pattle;
	pattle.setColor(QPalette::Background, QColor(0, 0, 0, 255));
	setPalette(pattle);

	m_exit->show();
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "res/close.png");
	QObject::connect(m_exit, &COriginalButton::clicked, this, &StockDrawWidget::reject);
	setCustomerTitleBarHeight(35);
	resize(1280, 720);
}

void StockDrawWidget::setAvgParam(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate)
{
	std::map<IntDateTime, std::vector<BigNumber>> marketTemp;
	marketTemp["2019-09-12"].push_back(109.28);
	marketTemp["2019-09-12"].push_back(109.28);
	marketTemp["2019-09-12"].push_back(108.70);
	marketTemp["2019-09-12"].push_back(108.70);
	drawMarket(stock, marketTemp);
	return;
	StockIndicator::instance().loadCalcFromRedis(stock, beginDate, endDate);
	std::shared_ptr<StockAvgIndicator> spDrawAvg = StockIndicator::instance().avg();
	StockMarket stockMarket;
	stockMarket.loadFromRedis(stock, beginDate, endDate);
	stockMarket.load();

	std::map<IntDateTime, std::vector<BigNumber>> market;
	do 
	{
		IntDateTime date = stockMarket.date();
		std::shared_ptr<StockAvg> spStockAvg = spDrawAvg->day(date);
		std::vector<BigNumber>& dayMarket = market[date];
		dayMarket.push_back(spStockAvg->m_avg10_30);
		dayMarket.push_back(spStockAvg->m_avgHigh);
		dayMarket.push_back(spStockAvg->m_avgLow);
		dayMarket.push_back(spStockAvg->m_avg15_00);
	} while (stockMarket.next());
	drawMarket(stock, market);
}

void StockDrawWidget::drawMarket(const std::string& stock, const std::map<IntDateTime, std::vector<BigNumber>>& market)
{
	m_stock = stock;
	m_market = market;
}

void StockDrawWidget::drawOneKLine(QPainter& painter,
	int32_t x,
	int32_t width,
	int32_t y,
	int32_t height,
	const BigNumber& maxHigh,
	const BigNumber& minLow,
	const std::vector<BigNumber>& dayMarket)
{
	BigNumber dayAmplitude = dayMarket[1] - dayMarket[2];
	BigNumber amplitude = maxHigh - minLow;
	BigNumber drawHeight = dayAmplitude * height / amplitude.toPrec(16);
	BigNumber openy = y + (maxHigh - dayMarket[0]) * height / amplitude.toPrec(16);
	BigNumber highy = y + (maxHigh - dayMarket[1]) * height / amplitude.toPrec(16);
	BigNumber lowy = y + (maxHigh - dayMarket[2]) * height / amplitude.toPrec(16);
	BigNumber closey = y + (maxHigh - dayMarket[3]) * height / amplitude.toPrec(16);
	int32_t drawOpeny = atoi(openy.toString().c_str());
	int32_t drawHighy = atoi(highy.toString().c_str());
	int32_t drawLowy = atoi(lowy.toString().c_str());
	int32_t drawClosey = atoi(closey.toString().c_str());
	int32_t xLeft = x;
	int32_t xMid = width / 2 + x;
	int32_t xRight = x + width - 1;

	if (dayAmplitude >= 0)
	{
		if (dayAmplitude == 0)
		{
			painter.setPen("#FFFFFF");
		}
		else
		{
			painter.setPen("#FF6600");
		}
		QPoint dayMarketPoints[10];
		dayMarketPoints[0].setX(xMid);
		dayMarketPoints[0].setY(drawHighy);
		dayMarketPoints[1].setX(xMid);
		dayMarketPoints[1].setY(drawClosey);
		dayMarketPoints[2].setX(xLeft);
		dayMarketPoints[2].setY(drawClosey);
		dayMarketPoints[3].setX(xLeft);
		dayMarketPoints[3].setY(drawOpeny);
		dayMarketPoints[4].setX(xMid);
		dayMarketPoints[4].setY(drawOpeny);
		dayMarketPoints[5].setX(xMid);
		dayMarketPoints[5].setY(drawLowy);
		dayMarketPoints[6].setX(xMid);
		dayMarketPoints[6].setY(drawOpeny);
		dayMarketPoints[7].setX(xRight);
		dayMarketPoints[7].setY(drawOpeny);
		dayMarketPoints[8].setX(xRight);
		dayMarketPoints[8].setY(drawClosey);
		dayMarketPoints[9].setX(xMid);
		dayMarketPoints[9].setY(drawClosey);
		painter.drawPolyline(dayMarketPoints, 10);
	}
	else
	{
		painter.setPen("#0000FF");
		QPoint dayMarketPoints[10];
		dayMarketPoints[0].setX(xMid);
		dayMarketPoints[0].setY(drawHighy);
		dayMarketPoints[1].setX(xMid);
		dayMarketPoints[1].setY(drawOpeny);
		dayMarketPoints[2].setX(xLeft);
		dayMarketPoints[2].setY(drawOpeny);
		dayMarketPoints[3].setX(xLeft);
		dayMarketPoints[3].setY(drawClosey);
		dayMarketPoints[4].setX(xMid);
		dayMarketPoints[4].setY(drawClosey);
		dayMarketPoints[5].setX(xMid);
		dayMarketPoints[5].setY(drawLowy);
		dayMarketPoints[6].setX(xMid);
		dayMarketPoints[6].setY(drawClosey);
		dayMarketPoints[7].setX(xRight);
		dayMarketPoints[7].setY(drawClosey);
		dayMarketPoints[8].setX(xRight);
		dayMarketPoints[8].setY(drawOpeny);
		dayMarketPoints[9].setX(xMid);
		dayMarketPoints[9].setY(drawOpeny);
		painter.drawPolyline(dayMarketPoints, 10);
		painter.fillRect(xLeft, drawOpeny, width, drawOpeny - drawClosey, "#0000FF");
	}
}

void StockDrawWidget::getMaxHighMinLow(BigNumber& maxHigh, BigNumber& minLow)
{
	if (m_market.empty())
	{
		return;
	}
	maxHigh = 0;
	minLow = 1000000;
	for (auto itMarket = m_market.begin(); itMarket != m_market.end(); ++itMarket)
	{
		std::vector<BigNumber>& dayMarket = itMarket->second;
		if (maxHigh < dayMarket[1])
		{
			maxHigh = dayMarket[1];
		}
		if (minLow > dayMarket[2])
		{
			minLow = dayMarket[2];
		}
	}
}

void StockDrawWidget::resizeEvent(QResizeEvent* eve)
{
	DialogShow::resizeEvent(eve);
	int32_t width = this->width();
	int32_t height = this->height();
	m_exit->setGeometry(width - 26 - 5, 5, 26, 26);
}

void StockDrawWidget::paintEvent(QPaintEvent* eve)
{
	DialogShow::paintEvent(eve);
	QPainter painter(this);
	painter.setPen(QColor(255, 0, 0, 255));
	painter.drawText(QPoint(50, 50), QString::fromStdString(m_stock));

	int32_t x = 100;
	int32_t width = 5 * 2 + 1;
	int32_t y = 50;
	int32_t height = this->height() - y * 2;
	BigNumber maxHigh;
	BigNumber minLow;
	getMaxHighMinLow(maxHigh, minLow);
	int32_t space = 5;
	
	for (auto itMarket = m_market.begin(); itMarket != m_market.end(); ++itMarket)
	{
		std::vector<BigNumber>& dayMarket = itMarket->second;
		drawOneKLine(painter, x, width, y, height, maxHigh, minLow, dayMarket);
		x += (width + space);
	}
}