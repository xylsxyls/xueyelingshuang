#include "WaitingBox.h"
#include <QWindow>
#include <Windows.h>
#include <QApplication>
#include <QKeyEvent>
#include <QCloseEvent>
#include "Label.h"
#include "../CGeneralStyle.h"
#include <QPainter>

WaitingBox::WaitingBox(QDialog* parent):
m_cancelEscAltF4(false),
m_title(nullptr),
m_tip(nullptr),
m_sep(nullptr),
m_highLight(false),
m_timeRest(-1),
m_timeId(-1)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
}

WaitingBox::~WaitingBox()
{
	setParentWindow(nullptr);
	auto childs = children();
	for each (QObject* var in childs)
	{
		QWidget* widget = qobject_cast<QWidget*>(var);
		if (widget)
		{
			widget->installEventFilter(nullptr);
		}
	}
}

void WaitingBox::initForWaitingBox(int32_t dialogWidth, int32_t dialogHeight)
{
	installEventFilter(this);
	setWindowFlags(windowFlags() | Qt::Tool);
	resize(dialogWidth, dialogHeight);//411, 165340, 165
	setWindowTitle("title");
	m_title = addLabel(windowTitle(), QRect(17, 4, width() - 17 * 2, 27), QColor(163, 175, 191, 255));
	m_sep = addSeparator(QPoint(13, 33), width() - 13 * 2, true, QColor(16, 20, 31, 255), QColor(46, 52, 88, 255));
	m_tip = addTip("tip", QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	QObject::connect(this, &WaitingBox::ncActiveChanged, this, &WaitingBox::onNcActiveChanged, Qt::QueuedConnection);
	QObject::connect(this, &WaitingBox::exitSignal, this, &WaitingBox::onExitSignal);
	setAttribute(Qt::WA_NativeWindow);
}

Label* WaitingBox::addLabel(const QString& text, const QRect& rect, const QColor& textColor)
{
	Label* label = new Label(this);
	if (label == nullptr)
	{
		return nullptr;
	}
	label->setGeometry(rect);
	label->setText(text);
	label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	label->setTextColor(textColor);
	label->setFontFace(CGeneralStyle::instance()->font().family());
	label->setFontSize(14);
	return label;
}

Label* WaitingBox::addTip(const QString& text, const QRect& rect, const QColor& textColor)
{
	Label* tip = addLabel(text, rect, textColor);
	if (tip == nullptr)
	{
		return nullptr;
	}
	tip->setWordWrap(true);
	return tip;
}

Label* WaitingBox::addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor)
{
	Label* label = new Label(this);
	if (label == nullptr)
	{
		return nullptr;
	}

	label->setGeometry(point.x(), point.y(), isHorizon ? length : 2, isHorizon ? 2 : length);
	label->setBackgroundColor(downColor);
	Label* lab = new Label(label);
	if (lab == nullptr)
	{
		return nullptr;
	}
	lab->setGeometry(0, 0, isHorizon ? length : 1, isHorizon ? 1 : length);
	lab->setBackgroundColor(upColor);
	return label;
}

bool WaitingBox::check()
{
	return m_title != nullptr && m_tip != nullptr && m_sep != nullptr;
}

void WaitingBox::setParentWindow(QWindow* parent)
{
	QWindow* window = windowHandle();
	if (window == nullptr)
	{
		return;
	}

	window->setTransientParent(topWindowHandle(parent));
	return;
}

void WaitingBox::setTitle(const QString& title)
{
	if (!check())
	{
		return;
	}

	setWindowTitle(title);
	m_title->setText(title);
}

void WaitingBox::setTip(const QString& tip)
{
	if (!check())
	{
		return;
	}

	m_tip->setText(tip);
}

void WaitingBox::setTimeRest(int32_t timeRest)
{
	if (m_timeId != -1)
	{
		killTimer(m_timeId);
	}
	m_timeRest = timeRest;
}

void WaitingBox::setUserExit(bool userExit)
{
	m_cancelEscAltF4 = !userExit;
}

void WaitingBox::exit()
{
	emit exitSignal();
}

void WaitingBox::showEvent(QShowEvent* eve)
{
	if (m_timeRest > 0)
	{
		m_timeId = startTimer(m_timeRest);
	}

	QWindow* handle = windowHandle();
	if (handle != nullptr && handle->transientParent())
	{
		setWindowModality(Qt::WindowModal);
	}
	else
	{
		setWindowModality(Qt::ApplicationModal);
	}

	QDialog::showEvent(eve);
	raise();
}

void WaitingBox::timerEvent(QTimerEvent* eve)
{
	killTimer(m_timeId);
	reject();
	emit waitingBoxDone(this, TIMEOUT_EXIT);
	QDialog::timerEvent(eve);
}

void WaitingBox::keyPressEvent(QKeyEvent* eve)
{
	if ((m_cancelEscAltF4 && (eve->key() == Qt::Key_Escape)))
	{
		return;
	}
	if (m_cancelEscAltF4 == false && (eve->key() == Qt::Key_Escape))
	{
		if (m_timeId != -1)
		{
			killTimer(m_timeId);
		}
		emit waitingBoxDone(this, USER_EXIT);
	}
	QDialog::keyPressEvent(eve);
}

void WaitingBox::closeEvent(QCloseEvent* eve)
{
	if (m_cancelEscAltF4)
	{
		eve->ignore();
		return;
	}

	if (m_timeId != -1)
	{
		killTimer(m_timeId);
	}

	emit waitingBoxDone(this, USER_EXIT);
	QDialog::closeEvent(eve);
}

void WaitingBox::paintEvent(QPaintEvent* eve)
{
	QLinearGradient gradient(QPointF(0, 0), QPoint(0, height()));
	gradient.setColorAt(0.08, QColor(31, 37, 61, 255));
	gradient.setColorAt(0.50, QColor(29, 33, 50, 255));
	gradient.setColorAt(0.92, QColor(27, 29, 41, 255));

	gradient.setSpread(QGradient::PadSpread);

	QPainter painter(this);

	painter.fillRect(QRect(0, 0, width(), height()), gradient);
	if (m_highLight)
	{
		painter.setPen(QColor("#373e60"));
		raise();
	}
	else
	{
		painter.setPen(QColor("#4b5586"));
	}
	painter.drawRect(0, 0, width() - 1, height() - 1);

	QDialog::paintEvent(eve);
}

void WaitingBox::onNcActiveChanged(const bool& ncActive)
{
	m_highLight = ncActive;
	QDialog::repaint();
}

void WaitingBox::onExitSignal()
{
	if (m_timeId != -1)
	{
		killTimer(m_timeId);
	}
	
	done(CODE_EXIT);
	emit waitingBoxDone(this, CODE_EXIT);
}

QWindow* WaitingBox::topWindowHandle(QWindow* parent)
{
	if (parent == nullptr)
	{
		return nullptr;
	}
	QWindow* windowInAppHandle = nullptr;
	WId topLevelHandle = (WId)::GetAncestor(HWND(parent->winId()), GA_ROOT);
	QWidget* widget = QWidget::find(topLevelHandle);
	if (widget != nullptr)
	{
		setAttribute(Qt::WA_NativeWindow);
		return widget->windowHandle();
	}
	else
	{
		foreach(QWindow* windowInApp, qApp->allWindows())
		{
			if (windowInApp->winId() == topLevelHandle)
			{
				return windowInApp;
			}
		}
	}
	return nullptr;
}