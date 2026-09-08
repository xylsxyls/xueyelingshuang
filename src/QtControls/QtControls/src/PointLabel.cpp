#include "PointLabel.h"

PointLabel::PointLabel(QWidget* parent) :
Label(parent),
m_leftOrigin(0)
{

}

PointLabel::~PointLabel()
{

}

void PointLabel::setTextOrigin(qint32 origin, bool rePaint)
{
	m_leftOrigin = qMax(origin, 0);
	Label::setTextOrigin(m_leftOrigin, rePaint);
}

void PointLabel::setText(const QString& str)
{
	m_str = str.toStdWString();
	Label::setText(str);
}

QString PointLabel::text()
{
	return QString::fromStdWString(m_str);
}

void PointLabel::showEvent(QShowEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
	Label::showEvent(eve);
	QFontMetrics fontMetrics(font());
	QString strShow = Label::text();
	//获取之前设置的字符串的像素大小
	qint32 fontSize = fontMetrics.width(strShow);
	const qint32 textWidth = qMax(width() - m_leftOrigin, 0);
	if (fontSize > textWidth)
	{
		//返回一个带有省略号的字符串
		strShow = fontMetrics.elidedText(strShow, Qt::ElideRight, textWidth);
	}
	Label::setText(strShow);
}