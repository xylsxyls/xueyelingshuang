#include "LumaPlayerRecordingSaveView.h"
#include "QtControls/Label.h"
#include "QtControls/RadioButton.h"
#include "QtControls/PushButton.h"
#include "LumaPlayerButton.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>

bool LumaPlayerRecordingSaveView::initView(const DialogParam&)
{
    QPalette colors = palette();
    colors.setColor(QPalette::Window, g_config.m_helpBackground);
    colors.setColor(QPalette::WindowText, g_config.m_textColor);
    colors.setColor(QPalette::Text, g_config.m_textColor);
    setPalette(colors);
    setAutoFillBackground(true);
    setFont(QFont(g_config.m_fontFamily, g_config.m_fontSize));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 16, 20, 16);
    Label* title = new Label(this);
    title->setText(QStringLiteral("保存录音"));
    title->setTextColor(g_config.m_textColor);
    layout->addWidget(title);
    // Keep both choices inside the managed dialog, without native dropdown windows.
    QButtonGroup* audio = new QButtonGroup(this);
    QButtonGroup* format = new QButtonGroup(this);
    const QString labels[2][2] = {
        {QStringLiteral("仅麦克风人声"), QStringLiteral("人声 + 伴奏")},
        {QStringLiteral("MP3 音频"), QStringLiteral("MP4 视频")}};
    for (int group = 0; group < 2; ++group)
    {
        QHBoxLayout* row = new QHBoxLayout;
        QButtonGroup* choices = group == 0 ? audio : format;
        for (int option = 0; option < 2; ++option)
        {
            RadioButton* button = new RadioButton(this);
            button->setObjectName(QString("recording%1%2").arg(group == 0 ? "Audio" : "Format").arg(option));
            button->setText(labels[group][option]);
            choices->addButton(button, option);
            row->addWidget(button, 1);
        }
        choices->button(group == 0 ? 1 : 0)->setChecked(true);
        layout->addLayout(row);
    }
    QHBoxLayout* buttons = new QHBoxLayout;
    LumaPlayerButton* discard = new LumaPlayerButton(false, this);
    discard->setText(QStringLiteral("放弃"));
    LumaPlayerButton* cancel = new LumaPlayerButton(false, this);
    cancel->setText(QStringLiteral("取消"));
    LumaPlayerButton* save = new LumaPlayerButton(false, this);
    save->setFixedSize(g_config.m_helpButtonWidth, g_config.m_helpButtonHeight);
    cancel->setFixedSize(g_config.m_helpButtonWidth, g_config.m_helpButtonHeight);
    discard->setFixedSize(g_config.m_helpButtonWidth, g_config.m_helpButtonHeight);
    save->setText(QStringLiteral("保存"));
    buttons->addWidget(discard);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    layout->addLayout(buttons);
    connect(save, &QPushButton::clicked, this, [this, audio, format]()
        {
            notifyCloseRequested(ACCEPT_BUTTON, format->checkedId() * 2 + audio->checkedId());
        });
    connect(discard, &QPushButton::clicked, this, [this]()
        {
            notifyCloseRequested(IGNORE_BUTTON, 0);
        });
    connect(cancel, &QPushButton::clicked, this, [this]()
        {
            notifyCloseRequested(ERROR_RESULT, 0);
        });
    return true;
}

QSize LumaPlayerRecordingSaveView::preferredSize() const
{
    return QSize(360, 190);
}
CustomViewBase* LumaPlayerRecordingSaveFactory::createView(const DialogParam&)
{
    return new LumaPlayerRecordingSaveView;
}
void LumaPlayerRecordingSaveFactory::destroy(CustomViewFactory* factory)
{
    delete factory;
}