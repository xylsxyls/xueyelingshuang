#include "PopDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include <QPainter>
#include "DialogHelper.h"
#include "Separator.h"

PopDialog::PopDialog():
m_separator(nullptr),
m_highLight(false)
{
    m_separator = new Separator(this);
    if (!check())
    {
        return;
    }
    init();
}

void PopDialog::init()
{
    if (!check())
    {
        return;
    }
    setNativeWindow(true);
    setWindowFlags(windowFlags() | Qt::Tool);
    setCustomerTitleBarHeight(40);
    setPopButtonConfig(m_exit, "", QColor(), 0, 12);
    m_exit->setBkgMargins(0, 0);
    m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupCloseButton.png");
    DialogHelper::setSeparator(m_separator, true, QColor(16, 20, 31, 255), QColor(46, 52, 88, 255));
    m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    addListenKey(Qt::Key_Escape);

    QObject::connect(this, &COriginalDialog::ncActiveChanged, this, &PopDialog::onNcActiveChanged);
    QObject::connect(this, &DialogBase::timeUp, this, &PopDialog::onTimeUp);
}

bool PopDialog::check()
{
    return m_separator != nullptr && DialogShow::check();
}

void PopDialog::setWindowTiTle(const QString& title,
                               const QColor& color,
                               int32_t fontSize,
                               Qt::Alignment align,
                               int32_t origin,
                               const QString& fontName)
{
    DialogShow::setWindowTiTle(title, color, fontSize, align, origin, fontName);
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
    DialogShow::showEvent(eve);
    activateWindow();
    raise();
}

void PopDialog::resizeEvent(QResizeEvent* eve)
{
    DialogShow::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_time->setGeometry(QRect(width() - 125 - 10, 140, 125, 32));
    m_exit->setGeometry(QRect(width() - 3 - 30, 3, 30, 30));
    m_separator->setGeometry(13, 33, width() - 13 * 2, 2);
}

void PopDialog::closeEvent(QCloseEvent* eve)
{
    if (m_timeId != -1)
    {
        killTimer(m_timeId);
        m_timeId = -1;
    }
    DialogShow::closeEvent(eve);
}

void PopDialog::onNcActiveChanged(const bool& ncActive)
{
    m_highLight = ncActive;
    repaint();
}

void PopDialog::onTimeUp()
{
    close();
}

void PopDialog::endDialog()
{
    //childAt(mapFromGlobal(QWidget::cursor().pos()))
    auto itResult = m_mapResult.find(focusWidget());
    if (itResult != m_mapResult.end())
    {
        m_result = itResult->second;
        done(m_result);
        //setResult(m_result);
        close();
    }
}