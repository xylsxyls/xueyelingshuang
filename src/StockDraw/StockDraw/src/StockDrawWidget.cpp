#include "StockDrawWidget.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include <stdint.h>

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

void StockDrawWidget::setAvgParam()
{

}

void StockDrawWidget::resizeEvent(QResizeEvent* eve)
{
	DialogShow::resizeEvent(eve);
	int32_t width = this->width();
	int32_t height = this->height();
	m_exit->setGeometry(width - 26 - 5, 5, 26, 26);
}
