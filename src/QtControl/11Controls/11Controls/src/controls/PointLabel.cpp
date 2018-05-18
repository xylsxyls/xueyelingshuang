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

void PointLabel::setTextOrigin(qint32 origin, bool rePaint)
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
	qint32 fontSize = fontMetrics.width(strShow);
	if (fontSize > width() - m_leftOrigin)
	{
		//����һ������ʡ�Ժŵ��ַ���
		strShow = fontMetrics.elidedText(strShow, Qt::ElideRight, width() - m_leftOrigin);
	}
	Label::setText(strShow);
}