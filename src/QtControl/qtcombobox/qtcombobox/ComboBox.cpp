#include "ComboBox.h"
#include <stdint.h>
#include "QssString.h"
#include "CStringManager.h"
#include <qglobal.h>
#include "QssHelper.h"
#include "NoFocusFrameDelegate.h"

ComboBox::ComboBox(QWidget* parent) :
QComboBox(parent),
m_hasSetBorderRadius(false),
m_hasSetBackgroundColor(false),
m_hasSetBorderWidth(false),
m_hasSetBorderImg(false),
m_hasSetBorderColor(false),
m_loadBorderImgSuccess(false),
m_hasSetDropDownWidth(false),
m_dropDownWidth(false),
m_hasSetDropDownHeight(false),
m_hasSetDropDownImg(false),
m_hasSetDropDownBorderWidth(false),
m_hasSetTextColor(false),
m_hasSetFontName(false),
m_hasSetFontSize(false),
m_hasSetTextOrigin(false),
m_hasSetListOrigin(false)
{
	m_listWidget = new ListWidget;
	setModel(m_listWidget->model());
	setView(m_listWidget);
	setItemDelegate(new NoFocusFrameDelegate);
}

void ComboBox::setBorderRadius(int32_t radius, bool rePaint)
{
	m_hasSetBorderRadius = true;
	m_borderRadius = radius;
	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setBackgroundColor(const QColor& normalColor,
								  const QColor& hoverColor,
								  const QColor& pressedColor,
								  const QColor& disabledColor,
								  bool rePaint)
{
	m_hasSetBackgroundColor = true;

	m_backgroundColorMap[NORMAL] = normalColor;
	m_backgroundColorMap[HOVER] = hoverColor;
	m_backgroundColorMap[PRESSED] = pressedColor;
	m_backgroundColorMap[DISABLED] = disabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setBorderWidth(int32_t width, bool rePaint)
{
	m_hasSetBorderWidth = true;
	m_borderWidth = width;
	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setBorderColor(const QColor& normalColor,
						      const QColor& hoverColor,
							  const QColor& pressedColor,
							  const QColor& disabledColor,
							  bool rePaint)
{
	m_hasSetBorderColor = true;

	m_borderColorMap[NORMAL] = normalColor;
	m_borderColorMap[HOVER] = hoverColor;
	m_borderColorMap[PRESSED] = pressedColor;
	m_borderColorMap[DISABLED] = disabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setBorderImage(const QString& borderImgPath,
							  int32_t borderImgStateCount,
							  int32_t borderImgNormal,
							  int32_t borderImgHover,
							  int32_t borderImgPressed,
							  int32_t borderImgDisabled,
							  bool rePaint)
{
	if (borderImgNormal > borderImgStateCount ||
		borderImgHover > borderImgStateCount ||
		borderImgPressed > borderImgStateCount ||
		borderImgDisabled > borderImgStateCount)
	{
		return;
	}

	m_hasSetBorderImg = true;

	m_loadBorderImgSuccess = QssHelper::GetPicHeight(borderImgPath.toStdWString(), borderImgStateCount, m_vecBorderImgHeight);
	m_borderImgStateCount = borderImgStateCount;

	m_borderImgPath = borderImgPath.toStdWString();
	m_borderImgMap[NORMAL] = borderImgNormal;
	m_borderImgMap[HOVER] = borderImgHover;
	m_borderImgMap[PRESSED] = borderImgPressed;
	m_borderImgMap[DISABLED] = borderImgDisabled;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setDropDownWidth(int32_t width, bool rePaint)
{
	m_hasSetDropDownWidth = true;
	m_dropDownWidth = width;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setDropDownHeight(int32_t width, bool rePaint)
{
	m_hasSetDropDownHeight = true;
	m_dropDownHeight = width;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setDropDownBorderWidth(int32_t width, bool rePaint)
{
	m_hasSetDropDownBorderWidth = true;
	m_dropDownBorderWidth = width;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setDropDownImage(const QString& dropDownImgPath, bool rePaint)
{
	m_hasSetDropDownImg = true;
	m_dropDownImgPath = dropDownImgPath.toStdWString();

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setTextColor(const QColor& normalColor,
							const QColor& hoverColor,
							const QColor& pressedColor,
							const QColor& disabledColor,
							bool rePaint)
{
	m_hasSetTextColor = true;

	m_itemTextColorMap[NORMAL] = normalColor;
	m_itemTextColorMap[HOVER] = hoverColor;
	m_itemTextColorMap[PRESSED] = pressedColor;
	m_itemTextColorMap[DISABLED] = disabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setFontFace(const QString& fontName, bool rePaint)
{
	m_hasSetFontName = true;
	m_fontName = fontName.toStdWString();
	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setFontSize(int32_t fontSize, bool rePaint)
{
	m_hasSetFontSize = true;
	m_fontSize = fontSize;
	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setTextOrigin(int32_t origin, bool rePaint)
{
	m_hasSetTextOrigin = true;
	m_textOrigin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::setListOrigin(int32_t origin, bool rePaint)
{
	m_hasSetListOrigin = true;
	m_listOrigin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ComboBox::addItem(const QString& text)
{
	QListWidgetItem* widgetItem = new QListWidgetItem(m_listWidget);
	widgetItem->setText(text);
}

void ComboBox::updateStyle()
{
	QssString buttonStyle;
	std::wstring className = L"QComboBox";

	//下拉边框粗度设为0，因为QListWidget已有边框，此属性px无效
	buttonStyle[className](L" ", L"QAbstractItemView").AddKeyValue(L"border", L"none");

	//圆角半径
	if (m_hasSetBorderRadius)
	{
		buttonStyle[className].AddKeyValue(L"border-radius", CStringManager::Format(L"%dpx", m_borderRadius));
	}

	//背景颜色
	if (m_hasSetBackgroundColor)
	{
		for (int32_t index = NORMAL; index <= DISABLED; ++index)
		{
			buttonStyle[className](index).AddKeyValue(L"background-color", QssHelper::QColorToWString(m_backgroundColorMap[index]));
		}
	}

	//设置边框粗度
	if (m_hasSetBorderWidth)
	{
		buttonStyle[className].AddKeyValue(L"border", CStringManager::Format(L"%dpx solid", m_borderWidth));
	}

	//设置边框颜色
	if (m_hasSetBorderColor)
	{
		for (int32_t index = NORMAL; index <= DISABLED; ++index)
		{
			buttonStyle[className](index).AddKeyValue(L"border-color", QssHelper::QColorToWString(m_borderColorMap[index]));
		}
	}

	//设置背景颜色
	if (m_hasSetBorderImg && m_loadBorderImgSuccess)
	{
		for (int32_t itemIndex = NORMAL; itemIndex <= DISABLED; ++itemIndex)
		{
			std::wstring imageUrl = CStringManager::Format(L"url(%s) %d 0 %d 0 stretch stretch",
				m_borderImgPath.c_str(),
				m_vecBorderImgHeight[qMax(m_borderImgMap[itemIndex] - 1, 0)],
				m_vecBorderImgHeight[qMin(m_borderImgStateCount - m_borderImgMap[itemIndex], m_borderImgStateCount - 1)]);
			buttonStyle[className](itemIndex).AddKeyValue(L"border-image", imageUrl);
		}
	}

	//设置下拉箭头宽度
	if (m_hasSetDropDownWidth)
	{
		buttonStyle[className][L"drop-down"].AddKeyValue(L"width", CStringManager::Format(L"%dpx", m_dropDownWidth));
	}

	//设置下拉箭头高度
	if (m_hasSetDropDownHeight)
	{
		buttonStyle[className][L"drop-down"].AddKeyValue(L"height", CStringManager::Format(L"%dpx", m_dropDownHeight));
	}

	//设置下拉箭头边框宽度
	if (m_hasSetDropDownBorderWidth)
	{
		buttonStyle[className][L"drop-down"].AddKeyValue(L"border", CStringManager::Format(L"%dpx solid", m_dropDownBorderWidth));
	}

	//设置下拉箭头图片
	if (m_hasSetDropDownImg)
	{
		std::wstring imageUrl = CStringManager::Format(L"url(%s)", m_dropDownImgPath.c_str());
		buttonStyle[className][L"down-arrow"].AddKeyValue(L"image", imageUrl);
	}

	//文字颜色
	if (m_hasSetTextColor)
	{
		for (int32_t itemIndex = NORMAL; itemIndex <= DISABLED; ++itemIndex)
		{
			QColor& color = m_itemTextColorMap[itemIndex];
			buttonStyle[className](itemIndex).AddKeyValue(L"color", QssHelper::QColorToWString(color));
		}
	}

	//设置字体
	if (m_hasSetFontName)
	{
		buttonStyle[className].AddKeyValue(L"font-family", CStringManager::Format(L"'%s'", m_fontName.c_str()));
	}

	//字体大小
	if (m_hasSetFontSize)
	{
		buttonStyle[className].AddKeyValue(L"font-size", CStringManager::Format(L"%dpx", m_fontSize));
	}

	//文字偏移量
	if (m_hasSetTextOrigin)
	{
		buttonStyle[className].AddKeyValue(L"padding-left", CStringManager::Format(L"%dpx", m_textOrigin));
	}

	//下拉菜单纵向偏移量
	if (m_hasSetListOrigin)
	{
		buttonStyle[className](L" ", L"QAbstractItemView").AddKeyValue(L"margin-top", CStringManager::Format(L"%dpx", m_listOrigin));
	}
	
	setStyleSheet(QString::fromStdWString(buttonStyle.toWString()));
}

void ComboBox::showEvent(QShowEvent* eve)
{
	updateStyle();
	m_listWidget->repaint();
}

void ComboBox::repaint()
{
	updateStyle();
	m_listWidget->repaint();
}