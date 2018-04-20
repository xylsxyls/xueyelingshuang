#include "PopDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include <QPainter>

PopDialog::PopDialog()
{
    init();
}

void PopDialog::init()
{
    setNativeWindow(true);
    setEscAltF4Enable(false);
    setWindowFlags(windowFlags() | Qt::Tool);
    setCustomerTitleBarHeight(40);
    m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupCloseButton.png");
    //m_separator = addSeparator(QPoint(13, 33), width() - 13 * 2, true, QColor(16, 20, 31, 255), QColor(46, 52, 88, 255));
    m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void PopDialog::done(int32_t result)
{
    if (m_timeId != -1)
    {
        killTimer(m_timeId);
        m_timeId = -1;
    }
    DialogShow::done(result);
}

void PopDialog::paintEvent(QPaintEvent* eve)
{
    QLinearGradient gradient(QPointF(0, 0), QPoint(0, height()));
    gradient.setColorAt(0.08, QColor(31, 37, 61, 255));
    gradient.setColorAt(0.50, QColor(29, 33, 50, 255));
    gradient.setColorAt(0.92, QColor(27, 29, 41, 255));

    gradient.setSpread(QGradient::PadSpread);

    QPainter painter(this);
    painter.save();
    painter.fillRect(QRect(0, 0, width(), height()), gradient);
    if (m_highLight)
    {
        painter.setPen(QColor("#373e60"));
        raise();
    }
    else
    {
        painter.setPen(QColor("#4b5586"));
    }
    painter.drawRect(0, 0, width() - 1, height() - 1);
    painter.restore();
    QDialog::paintEvent(eve);
}

void PopDialog::showEvent(QShowEvent* eve)
{
    DialogBase::showEvent(eve);
    activateWindow();
    raise();
}

void PopDialog::resizeEvent(QResizeEvent* eve)
{
    DialogShow::resizeEvent(eve);
    m_time->setGeometry(QRect(width() - 125 - 10, 140, 125, 32));
    m_exit->setGeometry(QRect(width() - 3 - 30, 3, 30, 30));
}

void PopDialog::onNcActiveChanged(const bool& ncActive)
{
    m_highLight = ncActive;
    repaint();
}