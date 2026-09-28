#include "DialogShadow.h"
#include "DialogShadowConfig.h"
#include <QEvent>
#include <QApplication>
#include <QPainter>
#include <QPaintEvent>
#include <QtMath>
#include <algorithm>
#include <cmath>

DialogShadow::DialogShadow(QWidget* dialog) :
m_dialog(dialog),
m_size(0),
m_imageSize(0),
m_ratio(0),
m_margin(0),
m_outlineMove(false),
m_targetVisible(false)
{
    setParent(dialog);
    setObjectName(QStringLiteral("qtControlsDialogShadow"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint |
        Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);
    m_dialog->installEventFilter(this);
}

DialogShadow::~DialogShadow()
{
    m_dialog->removeEventFilter(this);
}

void DialogShadow::setShadowSize(qint32 size)
{
    m_size = (std::max)(0, (std::min)(static_cast<int>(size), static_cast<int>(DialogShadowConfig::kMaximumSize)));
    if (m_size == 0 && m_outlineMove)
    {
        finishOutlineMove(false);
    }
    synchronize();
}

void DialogShadow::synchronize()
{
    if (m_size <= 0 || !m_dialog->isVisible() || m_dialog->isMinimized() ||
        m_dialog->isMaximized() || m_dialog->isFullScreen())
    {
        hide();
        return;
    }
    if (m_outlineMove)
    {
        return;
    }
    updateImage(m_dialog->size(), m_dialog->devicePixelRatio());
    const QRect body(m_dialog->mapToGlobal(QPoint(0, 0)), m_dialog->size());
    setGeometry(body.adjusted(-m_margin, -m_margin, m_margin, m_margin));
    show();
    // Ownership keeps the tool above its dialog. Do not raise the group over
    // another application when a background dialog is moved programmatically.
    update();
}

void DialogShadow::beginOutlineMove(const QPoint& globalPos)
{
    m_outlineMove = true;
    m_targetVisible = false;
    m_dragStart = globalPos;
    m_dragOrigin = QRect(m_dialog->mapToGlobal(QPoint(0, 0)), m_dialog->size());
    m_dragTarget = m_dragOrigin;
    hide();
    m_dialog->grabMouse();
}

void DialogShadow::moveOutline(const QPoint& globalPos)
{
    if (!m_outlineMove || m_size <= 0)
    {
        return;
    }
    const QPoint delta = globalPos - m_dragStart;
    if (!m_targetVisible && delta.manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }
    m_targetVisible = true;
    m_dragTarget = m_dragOrigin.translated(delta);
    setGeometry(m_dragTarget);
    show();
    raise();
    update();
}

void DialogShadow::finishOutlineMove(bool accepted)
{
    m_outlineMove = false;
    m_targetVisible = false;
    m_dialog->releaseMouse();
    if (accepted)
    {
        m_dialog->move(m_dragTarget.topLeft());
    }
    synchronize();
}

bool DialogShadow::outlineMoveActive() const
{
    return m_outlineMove;
}

bool DialogShadow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_dialog)
    {
        switch (event->type())
        {
        case QEvent::Hide:
        {
            if (m_outlineMove)
            {
                finishOutlineMove(false);
            }
            hide();
            break;
        }
        case QEvent::WindowDeactivate:
        {
            if (m_outlineMove)
            {
                finishOutlineMove(false);
            }
            break;
        }
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::WindowStateChange:
        case QEvent::WindowActivate:
        case QEvent::ZOrderChange:
        {
            synchronize();
            break;
        }
        default:
        {
            break;
        }
        }
    }
    return false;
}

void DialogShadow::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    if (m_outlineMove)
    {
        if (m_targetVisible)
        {
            // Only the tracked target is drawn; no native XOR frame runs in parallel.
            painter.setPen(QPen(Qt::black, DialogShadowConfig::kOutlineWidth));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(rect().adjusted(1, 1, -1, -1));
        }
        return;
    }
    painter.drawImage(QPoint(0, 0), m_image);
}

qreal DialogShadow::coverage(qreal sample, qreal start, qreal end, qreal sigma) const
{
    // The erf difference is the convolution of an interval with a Gaussian.
    const qreal divisor = sigma * std::sqrt(2.0);
    return 0.5 * (std::erf((sample - start) / divisor) - std::erf((sample - end) / divisor));
}

void DialogShadow::updateImage(const QSize& bodySize, qreal ratio)
{
    if (m_bodySize == bodySize && m_imageSize == m_size && qFuzzyCompare(m_ratio, ratio))
    {
        return;
    }
    m_bodySize = bodySize;
    m_imageSize = m_size;
    m_ratio = ratio;
    const qreal sigma = m_size * DialogShadowConfig::kBroadSigmaFactor;
    const qreal offset = m_size * DialogShadowConfig::kDownwardOffsetFactor;
    m_margin = qCeil(sigma * DialogShadowConfig::kCutoffSigma + offset);
    m_image = QImage(qCeil((bodySize.width() + m_margin * 2) * ratio),
        qCeil((bodySize.height() + m_margin * 2) * ratio), QImage::Format_ARGB32_Premultiplied);
    if (m_image.isNull())
    {
        m_bodySize = QSize();
        return;
    }
    m_image.setDevicePixelRatio(ratio);
    m_image.fill(Qt::transparent);
    const qreal left = m_margin;
    const qreal top = m_margin;
    const qreal right = left + bodySize.width();
    const qreal bottom = top + bodySize.height();
    const QColor color = DialogShadowConfig::color();
    for (int y = 0; y < m_image.height(); ++y)
    {
        QRgb* scan = reinterpret_cast<QRgb*>(m_image.scanLine(y));
        const qreal py = (y + 0.5) / ratio;
        const qreal broadY = coverage(py, top + offset, bottom + offset, sigma);
        const qreal contactY = coverage(py, top + DialogShadowConfig::kContactOffset, bottom + DialogShadowConfig::kContactOffset, m_size);
        for (int x = 0; x < m_image.width(); ++x)
        {
            const qreal px = (x + 0.5) / ratio;
            if (px >= left && px < right && py >= top && py < bottom)
            {
                continue;
            }
            const qreal broad = broadY * coverage(px, left, right, sigma);
            const qreal contact = contactY * coverage(px, left, right, m_size);
            const int alpha = qRound(255 * (DialogShadowConfig::kBroadOpacity * broad + DialogShadowConfig::kContactOpacity * contact));
            scan[x] = qPremultiply(qRgba(color.red(), color.green(), color.blue(), alpha));
        }
    }
}