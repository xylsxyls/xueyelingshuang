#include "CheckBox.h"
#include <stdint.h>
#include "QssString.h"
#include "CStringManager.h"
#include "QssHelper.h"

CheckBox::CheckBox(QWidget* parent) :
QCheckBox(parent),
m_hasSetTextColor(false),
m_hasSetBackgroundColor(false),
m_hasSetTextOrigin(false),
m_hasSetIndicatorImg(false),
m_hasSetBackgroundImg(false),
m_hasSetFontSize(false),
m_hasSetFontFace(false),
m_loadIndicatorImgSuccess(false),
m_loadBackgroundImgSuccess(false)
{

}

void CheckBox::setIndicatorImage(const QString& indicatorImg,
								 int32_t indicatorImgStateCount,
								 int32_t indicatorImgNormal,
								 int32_t indicatorImgHover,
								 int32_t indicatorImgPressed,
								 int32_t indicatorImgDisabled,
								 int32_t indicatorImgCkNormal,
								 int32_t indicatorImgCkHover,
								 int32_t indicatorImgCkPressed,
								 int32_t indicatorImgCkDisabled,
								 bool rePaint)
{
	if (indicatorImgNormal > indicatorImgStateCount ||
		indicatorImgHover > indicatorImgStateCount ||
		indicatorImgPressed > indicatorImgStateCount ||
		indicatorImgDisabled > indicatorImgStateCount ||
		indicatorImgCkNormal > indicatorImgStateCount ||
		indicatorImgCkHover > indicatorImgStateCount ||
		indicatorImgCkPressed > indicatorImgStateCount ||
		indicatorImgCkDisabled > indicatorImgStateCount)
	{
		return;
	}

	m_hasSetIndicatorImg = true;
	m_indicatorImgPath = indicatorImg.toStdWString();
	m_indicatorImgStateCount = indicatorImgStateCount;

	m_loadIndicatorImgSuccess = QssHelper::GetPicHeight(m_indicatorImgPath, m_indicatorImgStateCount, m_vecIndicatorImgHeight);

	m_indicatorImgMap[UNCHECK][NORMAL] = indicatorImgNormal;
	m_indicatorImgMap[UNCHECK][HOVER] = indicatorImgHover;
	m_indicatorImgMap[UNCHECK][PRESSED] = indicatorImgPressed;
	m_indicatorImgMap[UNCHECK][DISABLED] = indicatorImgDisabled;
	m_indicatorImgMap[CHECK][NORMAL] = indicatorImgCkNormal;
	m_indicatorImgMap[CHECK][HOVER] = indicatorImgCkHover;
	m_indicatorImgMap[CHECK][PRESSED] = indicatorImgCkPressed;
	m_indicatorImgMap[CHECK][DISABLED] = indicatorImgCkDisabled;

	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::setBackgroundImage(const QString& backgroundImg,
								  int32_t backgroundImgStateCount,
								  int32_t backgroundImgNormal,
								  int32_t backgroundImgHover,
								  int32_t backgroundImgPressed,
								  int32_t backgroundImgDisabled,
								  int32_t backgroundImgCkNormal,
								  int32_t backgroundImgCkHover,
								  int32_t backgroundImgCkPressed,
								  int32_t backgroundImgCkDisabled,
								  bool rePaint)
{
	if (backgroundImgNormal > backgroundImgStateCount ||
		backgroundImgHover > backgroundImgStateCount ||
		backgroundImgPressed > backgroundImgStateCount ||
		backgroundImgDisabled > backgroundImgStateCount ||
		backgroundImgCkNormal > backgroundImgStateCount ||
		backgroundImgCkHover > backgroundImgStateCount ||
		backgroundImgCkPressed > backgroundImgStateCount ||
		backgroundImgCkDisabled > backgroundImgStateCount)
	{
		return;
	}

	m_hasSetBackgroundImg = true;
	m_backgroundImgPath = backgroundImg.toStdWString();
	m_backgroundImgStateCount = backgroundImgStateCount;

	m_loadBackgroundImgSuccess = QssHelper::GetPicHeight(m_backgroundImgPath, m_backgroundImgStateCount, m_vecBackgroundImgHeight);

	m_backgroundImgMap[UNCHECK][NORMAL] = backgroundImgNormal;
	m_backgroundImgMap[UNCHECK][HOVER] = backgroundImgHover;
	m_backgroundImgMap[UNCHECK][PRESSED] = backgroundImgPressed;
	m_backgroundImgMap[UNCHECK][DISABLED] = backgroundImgDisabled;
	m_backgroundImgMap[CHECK][NORMAL] = backgroundImgCkNormal;
	m_backgroundImgMap[CHECK][HOVER] = backgroundImgCkHover;
	m_backgroundImgMap[CHECK][PRESSED] = backgroundImgCkPressed;
	m_backgroundImgMap[CHECK][DISABLED] = backgroundImgCkDisabled;

	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::setFontFace(const QString& fontName, bool rePaint)
{
	m_hasSetFontFace = true;
	m_fontName = fontName.toStdWString();
	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::setFontSize(int32_t fontSize, bool rePaint)
{
	m_hasSetFontSize = true;
	m_fontSize = fontSize;
	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::setTextColor(const QColor& textNormalColor,
	 	 		 		    const QColor& textHoverColor,
						    const QColor& textPressedColor,
						    const QColor& textDisabledColor,
						    const QColor& textCkNormalColor,
						    const QColor& textCkHoverColor,
						    const QColor& textCkPressedColor,
						    const QColor& textCkDisabledColor,
							bool rePaint)
{
	m_hasSetTextColor = true;

	m_textColorMap[UNCHECK][NORMAL] = textNormalColor;
	m_textColorMap[UNCHECK][HOVER] = textHoverColor;
	m_textColorMap[UNCHECK][PRESSED] = textPressedColor;
	m_textColorMap[UNCHECK][DISABLED] = textDisabledColor;
	m_textColorMap[CHECK][NORMAL] = textCkNormalColor;
	m_textColorMap[CHECK][HOVER] = textCkHoverColor;
	m_textColorMap[CHECK][PRESSED] = textCkPressedColor;
	m_textColorMap[CHECK][DISABLED] = textCkDisabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::setBackgroundColor(const QColor& backgroundNormalColor,
	 	 		 				  const QColor& backgroundHoverColor,
								  const QColor& backgroundPressedColor,
								  const QColor& backgroundDisabledColor,
								  const QColor& backgroundCkNormalColor,
								  const QColor& backgroundCkHoverColor,
								  const QColor& backgroundCkPressedColor,
								  const QColor& backgroundCkDisabledColor,
								  bool rePaint)
{
	m_hasSetBackgroundColor = true;

	m_backgroundColorMap[UNCHECK][NORMAL] = backgroundNormalColor;
	m_backgroundColorMap[UNCHECK][HOVER] = backgroundHoverColor;
	m_backgroundColorMap[UNCHECK][PRESSED] = backgroundPressedColor;
	m_backgroundColorMap[UNCHECK][DISABLED] = backgroundDisabledColor;
	m_backgroundColorMap[CHECK][NORMAL] = backgroundCkNormalColor;
	m_backgroundColorMap[CHECK][HOVER] = backgroundCkHoverColor;
	m_backgroundColorMap[CHECK][PRESSED] = backgroundCkPressedColor;
	m_backgroundColorMap[CHECK][DISABLED] = backgroundCkDisabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::setTextOrigin(int32_t origin, bool rePaint)
{
	m_hasSetTextOrigin = true;
	m_textOrigin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void CheckBox::repaint()
{
	updateStyle();
}

void CheckBox::updateStyle()
{
	QssString buttonStyle;
	std::wstring className = L"QCheckBox";

	//设置字体
	if(m_hasSetFontFace)
	{
		buttonStyle[className].AddKeyValue(L"font-family",CStringManager::Format(L"'%s'", m_fontName.c_str()));
	}

	//文本偏移
	if(m_hasSetTextOrigin)
	{
		buttonStyle[className].AddKeyValue(L"spacing",CStringManager::Format(L"%dpx", m_textOrigin));
	}
	
	//字体大小
	if(m_hasSetFontSize)
	{
		buttonStyle[className].AddKeyValue(L"font-size",CStringManager::Format(L"%dpx", m_fontSize));
	}

	//指示器背景图片
	if(m_hasSetIndicatorImg && m_loadIndicatorImgSuccess)
	{
		for (int32_t index = UNCHECK; index <= CHECK; ++index)
		{
			for (int32_t itemIndex = NORMAL; itemIndex <= DISABLED; ++itemIndex)
			{
				std::wstring imageUrl = CStringManager::Format(L"url(%s) %d 0 %d 0 stretch stretch",
					m_indicatorImgPath.c_str(),
					m_vecIndicatorImgHeight[qMax(m_indicatorImgMap[index][itemIndex] - 1, 0)],
					m_vecIndicatorImgHeight[qMin(m_indicatorImgStateCount - m_indicatorImgMap[index][itemIndex], m_indicatorImgStateCount - 1)]);
				buttonStyle[className][L"indicator"](index)(itemIndex).AddKeyValue(L"border-image", imageUrl);
			}
		}
	}
	
	//整体背景图片
	if(m_hasSetBackgroundImg && m_loadBackgroundImgSuccess)
	{
		for (int32_t index = UNCHECK; index <= CHECK; ++index)
		{
			for (int32_t itemIndex = NORMAL; itemIndex <= DISABLED; ++itemIndex)
			{
				std::wstring imageUrl = CStringManager::Format(L"url(%s) %d 0 %d 0 stretch stretch",
					m_backgroundImgPath.c_str(),
					m_vecBackgroundImgHeight[qMax(m_backgroundImgMap[index][itemIndex] - 1, 0)],
					m_vecBackgroundImgHeight[qMin(m_backgroundImgStateCount - m_backgroundImgMap[index][itemIndex], m_backgroundImgStateCount - 1)]);
				buttonStyle[className](index)(itemIndex).AddKeyValue(L"border-image", imageUrl);
			}
		}
	}
	
	//整体背景颜色
	if(m_hasSetBackgroundColor)
	{
		for (int32_t index = UNCHECK; index <= CHECK; ++index)
		{
			for (int32_t itemIndex = NORMAL; itemIndex <= DISABLED; ++itemIndex)
			{
				QColor& color = m_backgroundColorMap[index][itemIndex];
				buttonStyle[className](index)(itemIndex).AddKeyValue(L"background-color", QssHelper::QColorToWString(color));
			}
		}
	}
	
	//文字颜色
	if(m_hasSetTextColor)
	{
		for (int32_t index = UNCHECK; index <= CHECK; ++index)
		{
			for (int32_t itemIndex = NORMAL; itemIndex <= DISABLED; ++itemIndex)
			{
				QColor& color = m_textColorMap[index][itemIndex];
				buttonStyle[className](index)(itemIndex).AddKeyValue(L"color", QssHelper::QColorToWString(color));
			}
		}
	}

	setStyleSheet(QString::fromStdWString(buttonStyle.toWString()));
}

void CheckBox::showEvent(QShowEvent* eve)
{
	updateStyle();
}