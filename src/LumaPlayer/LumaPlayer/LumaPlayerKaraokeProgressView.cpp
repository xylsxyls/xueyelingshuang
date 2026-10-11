#include "LumaPlayerKaraokeProgressView.h"
#include "QtControls/Label.h"
#include "QtControls/ProgressBar.h"
#include "QtControls/PushButton.h"
#include "LumaPlayerButton.h"
#include "Config.h"
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QTimer>

LumaPlayerKaraokeProgressParam::LumaPlayerKaraokeProgressParam(LumaPlayerKaraokeJob* task) :
CustomDialogParam(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 1)),
m_task(task)
{

}

LumaPlayerKaraokeProgressView::LumaPlayerKaraokeProgressView(LumaPlayerKaraokeJob* task) :
m_task(task)
{

}

bool LumaPlayerKaraokeProgressView::initView(const DialogParam&)
{
    if (m_task == nullptr)
    {
        return false;
    }
    QPalette colors = palette();
    colors.setColor(QPalette::Window, g_config.m_helpBackground);
    colors.setColor(QPalette::WindowText, g_config.m_textColor);
    colors.setColor(QPalette::Text, g_config.m_textColor);
    colors.setColor(QPalette::Highlight, g_config.m_themeColor);
    setPalette(colors);
    setAutoFillBackground(true);
    setFont(QFont(g_config.m_fontFamily, g_config.m_fontSize));
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    Label* title = new Label(this);
    title->setText(m_task->title());
    title->setTextColor(g_config.m_textColor);
    ProgressBar* progress = new ProgressBar(this);
    progress->setRange(0, 100);
    progress->setValue(0);
    progress->setItemBackgroundColor(g_config.m_themeColor);
    progress->setTextColor(g_config.m_textColor);
    LumaPlayerButton* cancel = new LumaPlayerButton(false, this);
    cancel->setFixedHeight(g_config.m_helpButtonHeight);
    cancel->setText(QStringLiteral("取消"));
    layout->addWidget(title);
    layout->addWidget(progress);
    layout->addWidget(cancel);
    connect(cancel, &QPushButton::clicked, this, [this, cancel, title]()
        {
            if (m_task)
            {
                m_task->StopTask();
            }
            cancel->setEnabled(false);
            title->setText(QStringLiteral("正在取消，请稍候"));
        });
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, progress, timer]()
        {
            if (!m_task)
            {
                timer->stop();
                notifyCloseRequested(ERROR_RESULT, 0);
                return;
            }
            progress->setValue(m_task->progress());
            if (m_task->done())
            {
                timer->stop();
                const DialogResult result = m_task->succeeded() ? ACCEPT_BUTTON : IGNORE_BUTTON;
                m_task = nullptr;
                notifyCloseRequested(result, 0);
            }
        });
    window()->installEventFilter(this);
    timer->start(100);
    return true;
}

QSize LumaPlayerKaraokeProgressView::preferredSize() const
{
    return QSize(360, 160);
}

bool LumaPlayerKaraokeProgressView::eventFilter(QObject* object, QEvent* event)
{
    if (object == window() && (event->type() == QEvent::Close || (event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)) &&
        m_task && !m_task->done())
    {
        m_task->StopTask();
        event->ignore();
        return true;
    }
    return CustomViewBase::eventFilter(object, event);
}

CustomViewBase* LumaPlayerKaraokeProgressFactory::createView(const DialogParam& param)
{
    const LumaPlayerKaraokeProgressParam* value = dynamic_cast<const LumaPlayerKaraokeProgressParam*>(&param);
    return value ? new LumaPlayerKaraokeProgressView(value->m_task) : nullptr;
}

void LumaPlayerKaraokeProgressFactory::destroy(CustomViewFactory* factory)
{
    delete factory;
}