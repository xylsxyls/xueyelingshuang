#include "DialogBase.h"
#include "Label.h"
#include <QPainter>
#include <QPaintEvent>
#include <QWindow>
#include <QKeyEvent>

DialogBase::DialogBase():
m_timeId(-1),
m_escEnable(true),
m_timeRest(-1),
m_title(nullptr),
m_shadowEnabled(false),
m_shadowSize(0)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    m_title = new Label(this);
    m_title->setObjectName(QStringLiteral("dialogTitle"));
}

DialogBase::~DialogBase()
{
    auto childs = children();
    for (auto itChild = childs.begin(); itChild != childs.end(); ++itChild)
    {
        QObject* var = *itChild;
        QWidget* widget = qobject_cast<QWidget*>(var);
        if (widget)
        {
            widget->removeEventFilter(this);
        }
    }
}

void DialogBase::setNativeWindow(bool hasHandle)
{
    setAttribute(Qt::WA_NativeWindow, hasHandle);
}

void DialogBase::setTimeRest(qint32 timeOut)
{
    m_timeRest = timeOut;
	if (m_timeId != -1)
	{
		killTimer(m_timeId);
		m_timeId = -1;
	}
}

void DialogBase::setEscEnable(bool enable)
{
    m_escEnable = enable;
}

bool DialogBase::escEnable()
{
    return m_escEnable;
}

void DialogBase::setEscAltF4Enable(bool enable)
{
    setEscEnable(enable);
    setAltF4Enable(enable);
}

qint32 DialogBase::exec()
{
    setWindowModality((transientWindow() != nullptr) ? Qt::WindowModal : Qt::ApplicationModal);
    if (m_timeRest == 0)
    {
        return 0;
    }
    return COriginalDialog::exec();
}

void DialogBase::show()
{
    if (m_timeRest == 0)
    {
        return;
    }
    COriginalDialog::show();
}

void DialogBase::listenAllControls()
{
    auto childs = children();
    for (auto itChild = childs.begin(); itChild != childs.end(); ++itChild)
    {
        QObject* var = *itChild;
        QWidget* widget = qobject_cast<QWidget*>(var);
        if (widget)
        {
            widget->installEventFilter(this);
        }
    }
}

void DialogBase::addListenKey(Qt::Key key)
{
    m_listenKey.push_back(key);
}

void DialogBase::setWindowTitle(const QString& title,
                                const QColor& color,
                                qint32 fontSize,
                                Qt::Alignment align,
                                qint32 origin,
                                const QString& fontName)
{
    if (!check())
    {
        return;
    }
    COriginalDialog::setWindowTitle(title);
    m_title->setText(title);
    m_title->setTextColor(color);
    m_title->setFontSize(fontSize);
    m_title->setAlignment(align);
    m_title->setTextOrigin(origin);
    m_title->setFontFace(fontName);
}

void DialogBase::setWindowShadow(bool enabled, qint32 size)
{
    m_shadowEnabled = enabled && size > 0;
    m_shadowSize = m_shadowEnabled ? qMax<qint32>(size, 0) : 0;
    // Keep the parameters for API and future implementation compatibility.
    // Shadow rendering is intentionally disabled for the current rectangular
    // dialog style.
    setAttribute(Qt::WA_TranslucentBackground, false);
    update();
}

bool DialogBase::windowShadowEnabled() const
{
    return m_shadowEnabled;
}

qint32 DialogBase::windowShadowSize() const
{
    return m_shadowSize;
}

void DialogBase::showEvent(QShowEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
	if (m_timeRest > 0)
	{
		if (m_timeId != -1)
		{
			killTimer(m_timeId);
			m_timeId = -1;
		}
		m_timeId = startTimer(1000);
		emit timeRest(m_timeRest);
	}

    if (!m_listenKey.empty())
    {
        listenAllControls();
    }

	QDialog::showEvent(eve);
	raise();
	emit alreadyShown();
}

void DialogBase::timerEvent(QTimerEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
	if (m_timeId == -1 || eve->timerId() != m_timeId)
	{
		QDialog::timerEvent(eve);
		return;
	}
	--m_timeRest;
	emit timeRest(m_timeRest);
	if (m_timeRest == 0)
	{
        killTimer(m_timeId);
		m_timeId = -1;
        emit timeUp();
	}
}

void DialogBase::keyPressEvent(QKeyEvent* eve)
{
    if (eve == nullptr)
    {
        return;
    }

    switch (eve->key())
    {
    case Qt::Key_Escape:
    {
        eve->ignore();
        if (m_escEnable)
        {
            escEvent();
        }
        return;
    }
    default:
        break;
    }
    QDialog::keyPressEvent(eve);
}

bool DialogBase::eventFilter(QObject* tar, QEvent* eve)
{
    if (tar == nullptr || eve == nullptr)
    {
		return false;
    }
	bool res = COriginalDialog::eventFilter(tar, eve);
    if (eve->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(eve);
        auto result = std::find(m_listenKey.begin(), m_listenKey.end(), keyEvent->key());
        if (result != m_listenKey.end())
        {
            emit keyboardAccept(tar, static_cast<Qt::Key>(keyEvent->key()));
        }
        //Qt::Key_Space Qt::Key_Enter Qt::Key_Return Qt::Key_Escape
    }

    return res;
}

void DialogBase::resizeEvent(QResizeEvent* eve)
{
	if (eve == nullptr)
	{
		return;
    }
    COriginalDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    qint32 titleHeight = customerTitleBarHeight();
    const qint32 frame = 1;
    m_title->setGeometry(frame, frame,
        qMax(width() - (frame * 2), 0),
        qMax(titleHeight, 0));
    setCustomerTitleBarTop(frame);
}

void DialogBase::paintEvent(QPaintEvent* eve)
{
    Q_UNUSED(eve);
    QPainter painter(this);
    QColor background = palette().color(QPalette::Window);
    if (!background.isValid() || background.alpha() == 0)
    {
        background = QColor(245, 247, 251);
    }

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), background);
    // Frameless dialogs have no native non-client frame.  Keep a simple
    // rectangular client edge; the shadow parameters remain API-only until a
    // future QtControls implementation is enabled.
    painter.setPen(QPen(QColor(32, 38, 48), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void DialogBase::escEvent()
{
    close();
}

bool DialogBase::check()
{
    return m_title != nullptr;
}