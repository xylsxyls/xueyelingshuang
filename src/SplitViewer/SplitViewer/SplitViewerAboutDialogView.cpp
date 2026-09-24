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
    setStyleSheet(Config::aboutDialogStyle());

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(30, 26, 30, 24);
    layout->setSpacing(0);

    QHBoxLayout* heading = new QHBoxLayout;
    heading->setSpacing(22);
    Label* logo = new Label(this);
    logo->setObjectName(QStringLiteral("aboutLogo"));
    logo->setFixedSize(106, 106);
    logo->setAlignment(Qt::AlignCenter);
    const QPixmap icon = QPixmap(QStringLiteral(":/icons/SplitViewer.png"));
    if (!icon.isNull())
    {
        logo->setPixmap(icon.scaled(94, 94, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    heading->addWidget(logo);

    QVBoxLayout* titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(4);
    titleLayout->addStretch(1);
    Label* title = new Label(this);
    title->setObjectName(QStringLiteral("aboutTitle"));
    title->setText(QStringLiteral("分屏看图"));
    titleLayout->addWidget(title);
    Label* version = new Label(this);
    version->setObjectName(QStringLiteral("aboutVersion"));
    version->setText(QStringLiteral("SplitViewer 1.0"));
    titleLayout->addWidget(version);
    titleLayout->addStretch(1);
    heading->addLayout(titleLayout, 1);
    layout->addLayout(heading);

    layout->addSpacing(22);
    Label* separator = new Label(this);
    separator->setObjectName(QStringLiteral("aboutSeparator"));
    separator->setFixedHeight(1);
    layout->addWidget(separator);
    layout->addSpacing(22);

    Label* message = new Label(this);
    message->setObjectName(QStringLiteral("aboutMessage"));
    message->setWordWrap(true);
    message->setAlignment(Qt::AlignCenter);
    message->setText(data->message.isEmpty() ? QStringLiteral("在同一块画布中组合多张图片，按分屏、图层和嵌入窗口自由呈现精彩内容。") : data->message);
    layout->addWidget(message, 1);

    QHBoxLayout* actions = new QHBoxLayout;
    actions->addStretch(1);
    m_closeButton = new PushButton(this);
    m_closeButton->setObjectName(QStringLiteral("aboutCloseButton"));
    m_closeButton->setText(QStringLiteral("知道了"));
    m_closeButton->setMinimumSize(128, 40);
    m_closeButton->setBkgColor(QColor(63, 159, 202), QColor(82, 173, 213), QColor(47, 131, 170), QColor(170, 188, 207),
        QColor(63, 159, 202), QColor(82, 173, 213), QColor(47, 131, 170), QColor(170, 188, 207));
    m_closeButton->setFontColor(QColor(255, 255, 255), QColor(255, 255, 255), QColor(255, 255, 255), QColor(120, 140, 160),
        QColor(255, 255, 255), QColor(255, 255, 255), QColor(255, 255, 255), QColor(120, 140, 160));
    m_closeButton->setBorderColor(QColor(50, 141, 181), QColor(63, 159, 202), QColor(40, 117, 149), QColor(145, 165, 185),
        QColor(50, 141, 181), QColor(63, 159, 202), QColor(40, 117, 149), QColor(145, 165, 185));
    m_closeButton->setBorderWidth(1);
    // Set the radius after all state colors so every PushButton state is rounded.
    m_closeButton->setBorderRadius(static_cast<quint32>(9));
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
    return Config::aboutDialogSize();
}

QWidget* SplitViewerAboutDialogView::defaultFocusWidget() const
{
    return m_closeButton;
}