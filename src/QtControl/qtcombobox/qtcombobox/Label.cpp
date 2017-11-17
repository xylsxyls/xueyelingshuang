#include "Label.h"

Label::Label(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QLabel", L"");
}

void Label::setFontFace(const QString& fontName, bool rePaint)
{
	m_fontName = fontName.toStdWString();
	ControlFont::setFontFace(fontName, rePaint);
}

void Label::setFontSize(int32_t fontSize, bool rePaint)
{
	m_fontSize = fontSize;
	ControlFont::setFontSize(fontSize, rePaint);
}

void Label::setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 			   const QColor& backgroundHoverColor,
							   const QColor& backgroundDisabledColor,
							   bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> backgroundColorMap;

	backgroundColorMap[NORMAL][NORMAL] = backgroundNormalColor;
	backgroundColorMap[NORMAL][HOVER] = backgroundHoverColor;
	backgroundColorMap[NORMAL][DISABLED] = backgroundDisabledColor;

	ControlBase::setColorStateMap(backgroundColorMap, L"background-color", false, rePaint);
}

void Label::setTextOrigin(int32_t origin, bool rePaint)
{
	m_leftOrigin = origin;
	ControlBase::setPxValue(L"padding-left", origin, false, rePaint);
}

void Label::setText(const QString& str)
{
	m_str = str.toStdWString();
	QFont font(QString::fromStdWString(m_fontName));
	font.setPixelSize(m_fontSize);
	QFontMetrics fontMetrics(font);
	//获取之前设置的字符串的像素大小
	int32_t fontSize = fontMetrics.width(str);
	QString strShow = str;
	if (fontSize > width() - m_leftOrigin)
	{
		//返回一个带有省略号的字符串
		strShow = fontMetrics.elidedText(str, Qt::ElideRight, width() - m_leftOrigin);
	}
	QLabel::setText(strShow);
}

QString Label::text()
{
	return QString::fromStdWString(m_str);
}