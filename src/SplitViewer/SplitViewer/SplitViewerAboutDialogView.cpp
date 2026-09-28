#include "SplitViewerAboutDialogView.h"

#include "Config.h"
#include "SplitViewerAboutDialogParam.h"
#include "QtControls/Label.h"
#include "QtControls/PushButton.h"
#include <QtGui/QPixmap>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>

SplitViewerAboutDialogView::SplitViewerAboutDialogView() :
m_closeButton(nullptr)
{

}

bool SplitViewerAboutDialogView::initView(const DialogParam& param)
{
    const SplitViewerAboutDialogParam* data = dynamic_cast<const SplitViewerAboutDialogParam*>(&param);
    if (data == nullptr || m_closeButton != nullptr)
    {
        return false;
    }
    setObjectName(QStringLiteral("splitViewerAboutView"));
    setStyleSheet(g_config.m_aboutDialogStyle);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(g_config.m_aboutMargins);
    layout->setSpacing(0);

    QHBoxLayout* heading = new QHBoxLayout;
    heading->setSpacing(g_config.m_aboutSpacing);
    Label* logo = new Label(this);
    logo->setObjectName(QStringLiteral("aboutLogo"));
    logo->setFixedSize(g_config.m_aboutLogoSize, g_config.m_aboutLogoSize);
    logo->setAlignment(Qt::AlignCenter);
    const QPixmap icon = QPixmap(g_config.m_iconPath);
    if (!icon.isNull())
    {
        logo->setPixmap(icon.scaled(g_config.m_aboutIconSize, g_config.m_aboutIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    heading->addWidget(logo);

    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(g_config.m_aboutTextSpacing);
    titleLayout->addStretch(1);
    Label* title = new Label(this);
    title->setObjectName(QStringLiteral("aboutTitle"));
    title->setText(g_config.m_applicationTitle);
    titleLayout->addWidget(title);
    Label* version = new Label(this);
    version->setObjectName(QStringLiteral("aboutVersion"));
    version->setText(g_config.m_versionText);
    titleLayout->addWidget(version);
    titleLayout->addStretch(1);
    heading->addLayout(titleLayout, 1);
    layout->addLayout(heading);

    layout->addSpacing(g_config.m_aboutSpacing);
    Label* separator = new Label(this);
    separator->setObjectName(QStringLiteral("aboutSeparator"));
    separator->setFixedHeight(g_config.m_aboutSeparatorHeight);
    layout->addWidget(separator);
    layout->addSpacing(g_config.m_aboutSpacing);

    Label* message = new Label(this);
    message->setObjectName(QStringLiteral("aboutMessage"));
    message->setWordWrap(true);
    message->setAlignment(Qt::AlignCenter);
    message->setText(data->message.isEmpty() ? g_config.m_aboutMessage : data->message);
    layout->addWidget(message, 1);

    QHBoxLayout* actions = new QHBoxLayout;
    actions->addStretch(1);
    m_closeButton = new PushButton(this);
    m_closeButton->setObjectName(QStringLiteral("aboutCloseButton"));
    m_closeButton->setText(g_config.m_acceptAboutText);
    m_closeButton->setMinimumSize(g_config.m_aboutAcceptSize);
    m_closeButton->setBkgColor(g_config.m_acceptColor, g_config.m_acceptHoverColor, g_config.m_acceptPressedColor, g_config.m_acceptDisabledColor,
        g_config.m_acceptColor, g_config.m_acceptHoverColor, g_config.m_acceptPressedColor, g_config.m_acceptDisabledColor);
    m_closeButton->setFontColor(g_config.m_acceptTextColor, g_config.m_acceptTextColor, g_config.m_acceptTextColor, g_config.m_acceptDisabledTextColor,
        g_config.m_acceptTextColor, g_config.m_acceptTextColor, g_config.m_acceptTextColor, g_config.m_acceptDisabledTextColor);
    m_closeButton->setBorderColor(g_config.m_acceptBorderColor, g_config.m_acceptColor, g_config.m_acceptPressedBorderColor, g_config.m_acceptDisabledBorderColor,
        g_config.m_acceptBorderColor, g_config.m_acceptColor, g_config.m_acceptPressedBorderColor, g_config.m_acceptDisabledBorderColor);
    m_closeButton->setBorderWidth(g_config.m_buttonBorderWidth);
    // Set the radius after all state colors so every PushButton state is rounded.
    m_closeButton->setBorderRadius(static_cast<quint32>(g_config.m_aboutAcceptRadius));
    actions->addWidget(m_closeButton);
    actions->addStretch(1);
    layout->addLayout(actions);

    connect(m_closeButton, &QPushButton::clicked, [this]()
    {
        notifyCloseRequested(ACCEPT_BUTTON, 0);
    });
    return true;
}

QSize SplitViewerAboutDialogView::preferredSize() const
{
    return g_config.m_aboutDialogSize;
}

QWidget* SplitViewerAboutDialogView::defaultFocusWidget() const
{
    return m_closeButton;
}