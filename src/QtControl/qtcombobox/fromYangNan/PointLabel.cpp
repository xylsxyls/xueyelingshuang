#include "PointLabel.h"

PointLabel::PointLabel(QWidget* parent) :
Label(parent),
m_leftOrigin(0)
{

}

PointLabel::~PointLabel()
{

}

void PointLabel::setTextOrigin(int32_t origin, bool rePaint)
{
	m_leftOrigin = origin;
	Label::setTextOrigin(origin, rePaint);
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
	Label::repaint();
	QFontMetrics fontMetrics(font());
	QString strShow = Label::text();
	//获取之前设置的字符串的像素大小
	int32_t fontSize = fontMetrics.width(strShow);
	if (fontSize > width() - m_leftOrigin)
	{
		//返回一个带有省略号的字符串
		strShow = fontMetrics.elidedText(strShow, Qt::ElideRight, width() - m_leftOrigin);
	}
	Label::setText(strShow);
}