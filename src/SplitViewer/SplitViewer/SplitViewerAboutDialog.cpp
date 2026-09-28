#include "SplitViewerAboutDialog.h"
#include "Config.h"

#include "SplitViewerAboutDialogParam.h"
#include "QtControls/Label.h"
#include "QtControls/PushButton.h"
#include <algorithm>
#include <QIcon>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>

SplitViewerAboutDialog::SplitViewerAboutDialog() :
CustomDialog()
{
    setExitVisible(true);
    if (m_exit != nullptr)
    {
        m_exit->setObjectName(QStringLiteral("dialogCloseButton"));
        QPixmap closeIcon(g_config.m_closeIconSize, g_config.m_closeIconSize);
        closeIcon.fill(Qt::transparent);
        QPainter iconPainter(&closeIcon);
        iconPainter.setRenderHint(QPainter::Antialiasing, true);
        iconPainter.setPen(QPen(g_config.m_closeColor, g_config.m_closeIconStroke, Qt::SolidLine, Qt::RoundCap));
        iconPainter.drawLine(QPointF(g_config.m_closeIconInset, g_config.m_closeIconInset), QPointF(g_config.m_closeIconSize - g_config.m_closeIconInset, g_config.m_closeIconSize - g_config.m_closeIconInset));
        iconPainter.drawLine(QPointF(g_config.m_closeIconSize - g_config.m_closeIconInset, g_config.m_closeIconInset), QPointF(g_config.m_closeIconInset, g_config.m_closeIconSize - g_config.m_closeIconInset));
        iconPainter.end();
        m_exit->setText(QString());
        m_exit->setIcon(QIcon(closeIcon));
        m_exit->setIconSize(closeIcon.size());
        m_exit->setTextAlign(QStringLiteral("center"));
        m_exit->setBkgColor(g_config.m_transparentColor, g_config.m_closeHoverColor, g_config.m_closePressedColor, g_config.m_transparentColor,
            g_config.m_transparentColor, g_config.m_closeHoverColor, g_config.m_closePressedColor, g_config.m_transparentColor);
        m_exit->setFontColor(g_config.m_closeColor, g_config.m_closeActiveTextColor, g_config.m_closeActiveTextColor, g_config.m_closeDisabledTextColor,
            g_config.m_closeColor, g_config.m_closeActiveTextColor, g_config.m_closeActiveTextColor, g_config.m_closeDisabledTextColor);
        m_exit->setBorderColor(g_config.m_transparentColor, g_config.m_closeHoverBorderColor, g_config.m_closePressedBorderColor, g_config.m_transparentColor,
            g_config.m_transparentColor, g_config.m_closeHoverBorderColor, g_config.m_closePressedBorderColor, g_config.m_transparentColor);
        m_exit->setBorderWidth(g_config.m_buttonBorderWidth);
        // Set the radius after all state colors so every PushButton state is rounded.
        m_exit->setBorderRadius(static_cast<quint32>(g_config.m_aboutCloseRadius));
        // Keep the icon aligned with the frame after its one-pixel downward move.
        m_exit->setMargins(static_cast<quint32>(0), static_cast<quint32>(0),
            static_cast<quint32>(0), static_cast<quint32>(0));
        QObject::connect(m_exit, &QPushButton::clicked, this, [this]()
        {
            setWindowResult(RIGHT_TOP_EXIT);
            close();
        });
    }
}

bool SplitViewerAboutDialog::initDialog(const DialogParam& param)
{
    if (!CustomDialog::initDialog(param))
    {
        return false;
    }
    Label* title = findChild<Label*>(QStringLiteral("dialogTitle"));
    if (title != nullptr)
    {
        title->setFontSize(g_config.m_aboutTitleFontSize);
        title->setFontBold(true);
    }
    const SplitViewerAboutDialogParam* aboutParam = dynamic_cast<const SplitViewerAboutDialogParam*>(&param);
    if (aboutParam != nullptr && aboutParam->centerRect.isValid())
    {
        move(aboutParam->centerRect.center() - QPoint(width() / 2, height() / 2));
    }
    updateTitleCloseButtonGeometry();
    return true;
}

void SplitViewerAboutDialog::resizeEvent(QResizeEvent* event)
{
    CustomDialog::resizeEvent(event);
    updateTitleCloseButtonGeometry();
}

void SplitViewerAboutDialog::updateTitleCloseButtonGeometry()
{
    if (m_exit == nullptr)
    {
        return;
    }
    const int buttonSize = g_config.m_aboutCloseSize;
    const int titleHeight = (std::max)(customerTitleBarHeight() - 2, buttonSize);
    m_exit->setGeometry(width() - buttonSize - g_config.m_aboutCloseRight, g_config.m_aboutCloseTop, buttonSize, (std::min)(titleHeight, buttonSize));
    m_exit->raise();
}