#include "Label.h"
#include <QEvent>

Label::Label(QWidget* parent) :
ControlShow(parent),
m_full(false),
m_hasSetFullScreen(false)
{
	ControlBase::setControlShow(this);
}

Label::~Label()
{
	if (m_hasSetFullScreen)
	{
		installEventFilter(nullptr);
	}
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
		installEventFilter(this);
		QObject::connect(this, &Label::doubleClicked, this, &Label::onDoubleClicked);
	}
}

bool Label::eventFilter(QObject* tar, QEvent* eve)
{
	bool res = QLabel::eventFilter(tar, eve);
	if (tar == nullptr || eve == nullptr)
	{
		return res;
	}
	if ((tar == this) && (eve->type() == QEvent::MouseButtonDblClick))
	{
		emit doubleClicked();
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