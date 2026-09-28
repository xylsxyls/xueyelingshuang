#include "PdfReaderDialog.h"

#include "PdfReaderDialogParam.h"
#include "QtControls/PushButton.h"
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>

PdfReaderDialog::PdfReaderDialog() :
    CustomDialog()
{
    if (m_exit != nullptr)
    {
        m_exit->setObjectName(QStringLiteral("dialogCloseButton"));
        QPixmap closeIcon(g_config.m_titleCloseIconSize, g_config.m_titleCloseIconSize);
        closeIcon.fill(Qt::transparent);
        QPainter iconPainter(&closeIcon);
        iconPainter.scale(g_config.m_titleCloseIconSize / 24.0, g_config.m_titleCloseIconSize / 24.0);
        iconPainter.setRenderHint(QPainter::Antialiasing, true);
        iconPainter.setPen(QPen(g_config.m_titleCloseColor, g_config.m_titleCloseStroke, Qt::SolidLine, Qt::RoundCap));
        iconPainter.drawLine(QPointF(5.0, 5.0), QPointF(19.0, 19.0));
        iconPainter.drawLine(QPointF(19.0, 5.0), QPointF(5.0, 19.0));
        iconPainter.end();
        m_exit->setText(QString());
        m_exit->setIcon(QIcon(closeIcon));
        m_exit->setIconSize(closeIcon.size());
        m_exit->setTextAlign(QStringLiteral("center"));
        m_exit->setBkgColor(QColor(0, 0, 0, 0), g_config.m_titleCloseHover, g_config.m_titleClosePressed, QColor(0, 0, 0, 0),
            QColor(0, 0, 0, 0), g_config.m_titleCloseHover, g_config.m_titleClosePressed, QColor(0, 0, 0, 0));
        m_exit->setFontColor(g_config.m_titleCloseColor, g_config.m_titleCloseActiveText, g_config.m_titleCloseActiveText, g_config.m_titleCloseDisabledText,
            g_config.m_titleCloseColor, g_config.m_titleCloseActiveText, g_config.m_titleCloseActiveText, g_config.m_titleCloseDisabledText);
        m_exit->setBorderColor(QColor(0, 0, 0, 0), g_config.m_titleCloseHoverBorder, g_config.m_titleClosePressedBorder, QColor(0, 0, 0, 0),
            QColor(0, 0, 0, 0), g_config.m_titleCloseHoverBorder, g_config.m_titleClosePressedBorder, QColor(0, 0, 0, 0));
        m_exit->setBorderWidth(1);
        m_exit->setBorderRadius(g_config.m_titleCloseRadius);
        m_exit->setMargins(static_cast<quint32>(0), static_cast<quint32>(0),
            static_cast<quint32>(0), static_cast<quint32>(0));
        QObject::connect(m_exit, &QPushButton::clicked, this, [this]()
        {
            setWindowResult(RIGHT_TOP_EXIT);
            close();
        });
    }
}

bool PdfReaderDialog::initDialog(const DialogParam& param)
{
    if (!CustomDialog::initDialog(param))
    {
        return false;
    }
    const PdfReaderDialogParam* readerParam = dynamic_cast<const PdfReaderDialogParam*>(&param);
    setExitVisible(readerParam != nullptr && readerParam->m_titleClose);
    updateTitleCloseButtonGeometry();
    return true;
}

void PdfReaderDialog::resizeEvent(QResizeEvent* event)
{
    CustomDialog::resizeEvent(event);
    updateTitleCloseButtonGeometry();
}

void PdfReaderDialog::updateTitleCloseButtonGeometry()
{
    if (m_exit == nullptr)
    {
        return;
    }
    const int buttonSize = g_config.m_titleCloseSize;
    m_exit->setGeometry(width() - buttonSize - g_config.m_titleCloseRight, g_config.m_titleCloseTop, buttonSize, buttonSize);
    m_exit->raise();
}