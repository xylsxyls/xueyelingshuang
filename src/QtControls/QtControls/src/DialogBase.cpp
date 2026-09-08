#include "DialogBase.h"
#include "Label.h"
#include <QWindow>
#include <QKeyEvent>

DialogBase::DialogBase():
m_timeId(-1),
m_escEnable(true),
m_timeRest(-1),
m_title(nullptr)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    m_title = new Label(this);
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
    m_title->setGeometry(0, 0, width(), titleHeight);
}

void DialogBase::escEvent()
{
    close();
}

bool DialogBase::check()
{
    return m_title != nullptr;
}