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
        QPixmap closeIcon(24, 24);
        closeIcon.fill(Qt::transparent);
        QPainter iconPainter(&closeIcon);
        iconPainter.setRenderHint(QPainter::Antialiasing, true);
        iconPainter.setPen(QPen(QColor(63, 84, 108), 1.2, Qt::SolidLine, Qt::RoundCap));
        iconPainter.drawLine(QPointF(5.0, 5.0), QPointF(19.0, 19.0));
        iconPainter.drawLine(QPointF(19.0, 5.0), QPointF(5.0, 19.0));
        iconPainter.end();
        m_exit->setText(QString());
        m_exit->setIcon(QIcon(closeIcon));
        m_exit->setIconSize(closeIcon.size());
        m_exit->setTextAlign(QStringLiteral("center"));
        m_exit->setBkgColor(QColor(0, 0, 0, 0), QColor(215, 228, 241), QColor(199, 216, 233), QColor(0, 0, 0, 0),
            QColor(0, 0, 0, 0), QColor(215, 228, 241), QColor(199, 216, 233), QColor(0, 0, 0, 0));
        m_exit->setFontColor(QColor(63, 84, 108), QColor(35, 61, 88), QColor(35, 61, 88), QColor(150, 165, 180),
            QColor(63, 84, 108), QColor(35, 61, 88), QColor(35, 61, 88), QColor(150, 165, 180));
        m_exit->setBorderColor(QColor(0, 0, 0, 0), QColor(191, 210, 227), QColor(176, 199, 219), QColor(0, 0, 0, 0),
            QColor(0, 0, 0, 0), QColor(191, 210, 227), QColor(176, 199, 219), QColor(0, 0, 0, 0));
        m_exit->setBorderWidth(1);
        m_exit->setBorderRadius(static_cast<quint32>(6));
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
    const bool isAbout = readerParam != nullptr &&
        readerParam->mode == PdfReaderDialogParam::Message &&
        readerParam->m_title == readerParam->config.aboutTitle &&
        readerParam->text == readerParam->config.aboutVersionText + QStringLiteral("\n") + readerParam->config.aboutText;
    const bool isRangeInput = readerParam != nullptr &&
        readerParam->mode == PdfReaderDialogParam::Input &&
        readerParam->m_title == readerParam->config.rangeTitle &&
        readerParam->text == readerParam->config.rangeExample;
    const bool hasTitleCloseButton = isAbout || isRangeInput;
    if (hasTitleCloseButton && readerParam->m_titleBarHeight > 0)
    {
        setCustomerTitleBarHeight(readerParam->m_titleBarHeight + 8);
        QResizeEvent relayoutEvent(size(), size());
        CustomDialog::resizeEvent(&relayoutEvent);
    }
    setExitVisible(hasTitleCloseButton);
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
    const int buttonSize = 34;
    const int titleHeight = qMax(customerTitleBarHeight() - 2, buttonSize);
    m_exit->setGeometry(width() - buttonSize - 4, 4, buttonSize, qMin(titleHeight, buttonSize));
    m_exit->raise();
}