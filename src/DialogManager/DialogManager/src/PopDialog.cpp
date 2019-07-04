#include "PopDialog.h"
#include "QtControls/COriginalButton.h"
#include "QtControls/CGeneralStyle.h"
#include <QPainter>
#include "DialogHelper.h"
#include "QtControls/Separator.h"

PopDialog::PopDialog():
m_separator(nullptr),
m_highLight(false),
m_normalColor("#373e60"),
m_highLightColor("#4b5586")
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
    setTouchBorderWidth(0);
    setPopButtonConfig(m_exit, "", QColor(), RIGHT_TOP_EXIT, 12);
    m_exit->setBkgMargins(0, 0);
    m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "res/Dialog/PopupCloseButton.png");
    DialogHelper::setSeparator(m_separator, true, QColor(16, 20, 31, 255), QColor(46, 52, 88, 255));
    
    DialogHelper::setLabel(m_time, "", QColor("#abb3d3"), 10);
    m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QObject::connect(this, &COriginalDialog::ncActiveChanged, this, &PopDialog::onNcActiveChanged);
    QObject::connect(this, &DialogBase::timeUp, this, &PopDialog::onTimeUp);
}

bool PopDialog::check()
{
    return m_separator != nullptr && DialogShow::check();
}

void PopDialog::setWindowTitle(const QString& title,
                               const QColor& color,
                               qint32 fontSize,
                               Qt::Alignment align,
                               qint32 origin,
                               const QString& fontName)
{
    DialogShow::setWindowTitle(title, color, fontSize, align, origin, fontName);
}

void PopDialog::setBorderNormalHighLightColor(const QColor& normalColor, const QColor& highLightColor)
{
    m_normalColor = normalColor;
    if (highLightColor != QColor(0, 0, 0, 0))
    {
        m_highLightColor = highLightColor;
    }
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
    painter.setPen(m_highLight ? m_highLightColor : m_normalColor);
    painter.drawRect(DialogHelper::rectValid(rect().adjusted(0, 0, -1, -1)));
    painter.restore();
    DialogShow::paintEvent(eve);
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
    m_time->setGeometry(QRect(width() - 125 - 10, height() - 18, 125, 18));
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
    m_highLight = !ncActive;
    repaint();
    raise();
}

void PopDialog::onTimeUp()
{
    if (m_result != nullptr)
    {
        *m_result = TIME_OUT;
    }
    done(TIME_OUT);
    close();
}

void PopDialog::endDialog()
{
    //childAt(mapFromGlobal(QWidget::cursor().pos()))
    auto itResult = m_mapResult.find(focusWidget());
    if (itResult != m_mapResult.end())
    {
        if (m_result != nullptr)
        {
            *m_result = itResult->second;
        }
        done(itResult->second);
        close();
    }
}