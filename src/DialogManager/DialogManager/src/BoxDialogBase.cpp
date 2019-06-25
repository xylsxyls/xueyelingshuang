#include "BoxDialogBase.h"
#include "QtControls/Label.h"
#include <QPainter>
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include "QtControls/Separator.h"
#include "core/coreAPI.h"
#include "DialogHelper.h"

BoxDialogBase::BoxDialogBase()
{
    if (!check())
    {
        return;
    }
	m_title->setBackgroundColor(QColor(67, 81, 117, 255));
	m_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_title->setTextOrigin(13);
	m_title->setTextColor(QColor(216, 218, 224, 255));

	m_exit->raise();
	m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");
	m_exit->setBorderRadius(0);

	m_separator->setVisible(false);

    setBorderNormalHighLightColor(QColor(79, 153, 222, 255));
	setCustomerTitleBarHeight(32);
	//setStyleSheet(".AccountDialogBase{background-color:rgba(44,52,74,255);border:1px solid;border-color:rgba(79,153,222,255);}");
}
void BoxDialogBase::paintEvent(QPaintEvent* eve)
{
	PopDialog::paintEvent(eve);
	QPainter painter(this);
    painter.save();
    painter.fillRect(DialogHelper::rectValid(QRect(1, 1, width() - 2, height() - 2)), QColor(44, 52, 74, 255));
    painter.restore();
}

void BoxDialogBase::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_title->setGeometry(QRect(1, 1, width() - 2, 31));
    m_exit->setGeometry(QRect(width() - 34, 1, 34, 31));
}

bool BoxDialogBase::check()
{
    return PopDialog::check();
}

