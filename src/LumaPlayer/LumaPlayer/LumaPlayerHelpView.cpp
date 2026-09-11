#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayerHelpView.h"
#include "LumaPlayerLogicController.h"
#include "Config.h"
#include "LumaPlayerButton.h"
#include "LogManager/LogManagerAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QWindow>

LumaPlayerHelpParam::LumaPlayerHelpParam(LumaPlayerLogicController* logic) :
CustomDialogParam(CUSTOM_DIALOG_TYPE_BEGIN),
m_logic(logic)
{

}

LumaPlayerHelpView::LumaPlayerHelpView(LumaPlayerLogicController* logic) :
m_playerWindow(nullptr),
m_centered(false),
m_titleBar(nullptr),
m_titleLabel(nullptr),
m_titleDragging(false),
m_titlePressGlobal(),
m_titlePressWindow(),
m_logic(logic),
m_confirm(nullptr),
m_token(0),
m_initialized(false)
{
    // 所有帮助视图均在GUI线程创建，以会话序号隔离延迟回报
    static uint64_t s_nextToken = 0;
    m_token = ++s_nextToken;
}

bool LumaPlayerHelpView::initView(const DialogParam& param)
{
    if (m_initialized)
    {
        return true;
    }
    if (m_logic == nullptr)
    {
        return false;
    }
    m_playerWindow = param.m_parent;
    window()->installEventFilter(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(g_config.m_titleMargin, g_config.m_titleButtonTop,
        g_config.m_titleMargin, g_config.m_helpMargin);
    layout->setSpacing(g_config.m_helpSpacing);
    QWidget* titleBar = new QWidget(this);
    m_titleBar = titleBar;
    titleBar->installEventFilter(this);
    titleBar->setFixedHeight(g_config.m_topOverlayHeight);
    QPalette titlePalette = titleBar->palette();
    titlePalette.setColor(QPalette::Window, g_config.m_overlayColor);
    titleBar->setPalette(titlePalette);
    titleBar->setAutoFillBackground(true);
    QHBoxLayout* titleRow = new QHBoxLayout(titleBar);
    titleRow->setContentsMargins(0, 0, 0, 0);
    QLabel* title = new QLabel(g_config.m_helpTitle, this);
    m_titleLabel = title;
    title->installEventFilter(this);
    QFont titleFont(g_config.m_fontFamily, g_config.m_helpTitleSize);
    titleFont.setBold(true);
    title->setFont(titleFont);
    LumaPlayerButton* close = new LumaPlayerButton(true, this);
    close->setText(g_config.m_closeText);
    close->setFixedSize(g_config.m_titleButtonSize, g_config.m_titleButtonSize);
    titleRow->addWidget(title);
    titleRow->addStretch();
    titleRow->addWidget(close);
    layout->addWidget(titleBar);
    QVBoxLayout* content = new QVBoxLayout();
    content->setContentsMargins(g_config.m_helpMargin - g_config.m_titleMargin, 0,
        g_config.m_helpMargin - g_config.m_titleMargin, 0);
    content->setSpacing(g_config.m_helpSpacing);
    QLabel* product = new QLabel(g_config.m_windowTitle, this);
    product->setFont(titleFont);
    product->setAlignment(Qt::AlignCenter);
    content->addWidget(product);
    QLabel* version = new QLabel(g_config.m_versionLabel, this);
    version->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    version->setFont(QFont(g_config.m_fontFamily, g_config.m_fontSize));
    content->addWidget(version);
    QLabel* instructions = new QLabel(QString::fromStdWString(CStringManager::Format(
        g_config.m_helpText.toStdWString().c_str(), g_config.m_keyboardSeekSeconds)), this);
    instructions->setWordWrap(true);
    instructions->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    instructions->setFont(QFont(g_config.m_fontFamily, g_config.m_fontSize));
    content->addWidget(instructions, 1);
    QHBoxLayout* footer = new QHBoxLayout();
    footer->addStretch();
    m_confirm = new LumaPlayerButton(false, this);
    m_confirm->setText(g_config.m_confirmText);
    m_confirm->setFixedSize(g_config.m_helpButtonWidth, g_config.m_helpButtonHeight);
    footer->addWidget(m_confirm);
    content->addLayout(footer);
    layout->addLayout(content, 1);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, g_config.m_helpBackground);
    palette.setColor(QPalette::WindowText, g_config.m_textColor);
    palette.setColor(QPalette::Button, g_config.m_themeColor);
    palette.setColor(QPalette::ButtonText, g_config.m_textColor);
    setPalette(palette);
    setAutoFillBackground(true);
    QObject::connect(close, &QPushButton::clicked, this, &LumaPlayerHelpView::requestClose);
    QObject::connect(m_confirm, &QPushButton::clicked, this, &LumaPlayerHelpView::requestClose);
    QObject::connect(m_logic, &LumaPlayerLogicController::resultReady,
        this, &LumaPlayerHelpView::onLogicResult, Qt::QueuedConnection);
    m_initialized = true;
    return true;
}

QSize LumaPlayerHelpView::preferredSize() const
{
    return QSize(g_config.m_helpWidth, g_config.m_helpHeight);
}

QWidget* LumaPlayerHelpView::defaultFocusWidget() const
{
    return m_confirm;
}

void LumaPlayerHelpView::requestClose()
{
    LumaPlayerLogicAction action;
    action.m_type = LumaActionHelpClose;
    action.m_revision = m_token;
    if (!m_logic->submit(action))
    {
        LOGWARNING("Help close task rejected token=%llu", static_cast<unsigned long long>(m_token));
    }
}

void LumaPlayerHelpView::onLogicResult(const LumaPlayerLogicResult& result)
{
    if ((result.m_type == LumaActionHelpClose && result.m_requestId == m_token) ||
        result.m_type == LumaActionShutdown)
    {
        notifyCloseRequested(ACCEPT_BUTTON, 0);
    }
}

bool LumaPlayerHelpView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == window() && event->type() == QEvent::Show && !m_centered &&
        !m_playerWindow.isNull())
    {
        const QRect owner = m_playerWindow->geometry();
        QWidget* dialog = window();
        dialog->move(owner.x() + (owner.width() - dialog->width()) / 2,
            owner.y() + (owner.height() - dialog->height()) / 2);
        m_centered = true;
    }
    if (watched != m_titleBar && watched != m_titleLabel)
    {
        return CustomViewBase::eventFilter(watched, event);
    }
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton)
        {
            m_titleDragging = true;
            m_titlePressGlobal = mouse->globalPos();
            m_titlePressWindow = window()->pos();
            return true;
        }
    }
    else if (event->type() == QEvent::MouseMove && m_titleDragging)
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        if ((mouse->buttons() & Qt::LeftButton) != 0)
        {
            window()->move(m_titlePressWindow + mouse->globalPos() - m_titlePressGlobal);
        }
        else
        {
            m_titleDragging = false;
        }
        return true;
    }
    else if (event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton && m_titleDragging)
        {
            m_titleDragging = false;
            return true;
        }
    }
    else if (event->type() == QEvent::UngrabMouse || event->type() == QEvent::Hide ||
        event->type() == QEvent::WindowDeactivate)
    {
        m_titleDragging = false;
    }
    return CustomViewBase::eventFilter(watched, event);
}