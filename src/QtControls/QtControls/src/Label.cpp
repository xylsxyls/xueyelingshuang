#include "Label.h"
#include <QEvent>
#include <QApplication>
#include <QMouseEvent>

Label::Label(QWidget* parent) :
ControlShow(parent),
m_full(false),
m_hasSetFullScreen(false)
{
	ControlBase::setControlShow(this);
	installEventFilter(this);
	setMouseTracking(true);
}

Label::~Label()
{
	installEventFilter(nullptr);
}

void Label::setLineHeight(qint32 lineHeight)
{
	setText("<html><head/><body><p style='line-height:" + QString::number(lineHeight) + "px;'>" + text().replace(" ", "&nbsp;") + "</p></body></html>");
}

void Label::setDoubleClickFullScreen()
{
	if (!m_hasSetFullScreen)
	{
		m_hasSetFullScreen = true;
		QObject::connect(this, &Label::doubleClicked, this, &Label::onDoubleClicked);
	}
}

void Label::mouseMoveEvent(QMouseEvent* eve)
{
	QLabel::mouseMoveEvent(eve);
	emit mouseMoved(QPoint(eve->x(), eve->y()));
}

bool Label::eventFilter(QObject* tar, QEvent* eve)
{
	bool res = QLabel::eventFilter(tar, eve);
	if (tar == nullptr || eve == nullptr)
	{
		return res;
	}
	if (tar == this)
	{
		switch (eve->type())
		{
		case QEvent::MouseButtonDblClick:
		{
			emit doubleClicked();
		}
		break;
		case QEvent::MouseButtonPress:
		{
			auto leftRight = qApp->mouseButtons();
			if (leftRight == Qt::LeftButton)
			{
				emit leftClicked();
			}
			else if (leftRight == Qt::RightButton)
			{
				emit rightClicked();
			}
		}
		break;
		default:
			break;
		}
	}
	return res;
}

void Label::onDoubleClicked()
{
	m_full = !m_full;
	if (m_full)
	{
		QWidget* parent = parentWidget();
		if (parent != nullptr)
		{
			m_rect = geometry();
		}
		setWindowFlags(Qt::Window);
		showFullScreen();
	}
	else
	{
		setWindowFlags(Qt::SubWindow);
		showNormal();
		QWidget* parent = parentWidget();
		if (parent != nullptr)
		{
			setGeometry(m_rect);
		}
	}
}