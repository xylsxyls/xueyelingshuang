#include "PointLabel.h"

PointLabel::PointLabel(QWidget* parent) :
Label(parent),
m_leftOrigin(0)
{
	INIT(L"");
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
	//��ȡ֮ǰ���õ��ַ��������ش�С
	int32_t fontSize = fontMetrics.width(strShow);
	if (fontSize > width() - m_leftOrigin)
	{
		//����һ������ʡ�Ժŵ��ַ���
		strShow = fontMetrics.elidedText(strShow, Qt::ElideRight, width() - m_leftOrigin);
	}
	Label::setText(strShow);
}