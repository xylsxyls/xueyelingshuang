#include "TipLabel.h"
#include <Windows.h>

TipLabel::TipLabel(QWidget* parent) :
Label(parent),
m_existTime(3),
m_timeId(-1),
m_maxWidth(500),
m_hasFocus(true),
m_bottomRight(0,0)
{
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool | Qt::ToolTip);
	setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
	setWordWrap(true);
	setMargin(3);
}

void TipLabel::setExistTime(qint32 time)
{
	m_existTime = time;
}

void TipLabel::setMaxWidth(qint32 maxWidth)
{
	m_maxWidth = maxWidth;
}

void TipLabel::setHasFocus(bool hasFocus)
{
	m_hasFocus = hasFocus;
}

void TipLabel::setBottomRight(const QPoint& bottomRight)
{
	m_bottomRight = bottomRight;
}

void TipLabel::showEvent(QShowEvent* eve)
{
	Label::showEvent(eve);
	m_timeId = startTimer(m_existTime * 1000);

	qint32 margin = 3;
	QFontMetrics fontMetrics(font());
	QString strShow = text();
	//��ȡ֮ǰ���õ��ַ��������ش�С
	qint32 textWidth = fontMetrics.width(strShow);
	qint32 textHeight = fontMetrics.height();
	qint32 tipWidth = textWidth + margin * 2;
	qint32 tipHeight = textHeight + margin * 2;
	if (tipWidth > m_maxWidth)
	{
		tipWidth = m_maxWidth;
		tipHeight = textHeight * (textWidth / (m_maxWidth - margin * 2) + 2) + margin * 2;
	}
	QPoint topLeft = m_bottomRight - QPoint(tipWidth, tipHeight);
	setGeometry(topLeft.x(), topLeft.y(), tipWidth, tipHeight);
}

void TipLabel::timerEvent(QTimerEvent* eve)
{
	Label::timerEvent(eve);
	if (m_timeId != -1)
	{
		killTimer(m_timeId);
		close();
	}
}

bool TipLabel::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
	bool res = Label::nativeEvent(eventType, message, result);
	if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
	{
		MSG* msg = static_cast<MSG *>(message);
		switch (msg->message)
		{
		case WM_ACTIVATE:
		{
			msg->lParam = -1; //DefWindowProc does not repaint the nonclient area to reflect the state change
			*result = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);

			if (msg->wParam == 0 && m_hasFocus)
			{
				close();
			}
			else if (m_hasFocus == false)
			{
				raise();
			}

			return true;
		}
		break;
		}
	}
	return res;
}