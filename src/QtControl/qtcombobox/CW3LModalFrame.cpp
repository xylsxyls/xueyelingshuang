#include "CW3LModalFrame.h"

#include <QMouseEvent>
#include <QPainter>
#include "CGeneralStyle.h"
#include <QDebug>

CW3LModalFrame::CW3LModalFrame(QWidget *parent)
	:COriginalDialog(parent)
{
	this->setWindowTitle("W3L Modal Frame");
	this->setFocusPolicy(Qt::ClickFocus);
	this->setCustomerTitleBarHeight(34);
	this->setWindowFlags(Qt::FramelessWindowHint);

	connect(this, &CW3LModalFrame::windowTitleChanged, this, &CW3LModalFrame::onWindowTitleChanged);
}

CW3LModalFrame::~CW3LModalFrame()
{

}

void CW3LModalFrame::paintEvent(QPaintEvent *e)
{
	QPainter p(this);

	//bkg
	p.fillRect(this->rect(), QColor(28,34,50));

	//border
	QRect borderRect = this->rect().adjusted(0,0,-1,-1);
	p.setPen(QPen(QColor(50,99,190),1));
	p.drawRect(borderRect);

	//title bar
	p.setPen(QColor("#d2b889"));
	p.drawLine(customerTitleBarRect().bottomLeft(), customerTitleBarRect().bottomRight());

	p.setFont(CGeneralStyle::instance()->font());
	p.drawText(customerTitleBarRect().adjusted(14,0,0,0), Qt::AlignLeft|Qt::AlignVCenter,
		this->windowTitle());
}


void CW3LModalFrame::closeEvent(QCloseEvent *e)
{
	COriginalDialog::closeEvent(e);
	emit closed();
	qDebug() << "w3l modal frame closed.";
}

void CW3LModalFrame::onWindowTitleChanged(const QString& s)
{
	this->update();
}

