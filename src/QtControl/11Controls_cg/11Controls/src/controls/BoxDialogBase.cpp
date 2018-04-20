#include "BoxDialogBase.h"
#include "Label.h"
#include <QPainter>
#include "COriginalButton.h"
#include "CGeneralStyle.h"

BoxDialogBase::BoxDialogBase()
{
    setChangeSizeEnable(false);
	m_title->setBackgroundColor(QColor(67, 81, 117, 255));
	m_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_title->setTextOrigin(13);
	m_title->setTextColor(QColor(216, 218, 224, 255));

	m_exit->raise();
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");
	m_exit->setBorderRadius(0);
	//m_separator->setVisible(false);
	//setStyleSheet(".AccountDialogBase{background-color:rgba(44,52,74,255);border:1px solid;border-color:rgba(79,153,222,255);}");
}
void BoxDialogBase::paintEvent(QPaintEvent* eve)
{
	PopDialog::paintEvent(eve);
	QPainter painter(this);
	painter.fillRect(QRect(0, 0, width(), height()), QColor(44, 52, 74, 255));
	painter.setPen(QColor(79, 153, 222, 255));
	painter.drawRect(0, 0, width() - 1, height() - 1);
}

void BoxDialogBase::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    m_title->setGeometry(QRect(1, 1, width() - 2, 31));
    m_exit->setGeometry(QRect(width() - 34, 1, 34, 31));
}

