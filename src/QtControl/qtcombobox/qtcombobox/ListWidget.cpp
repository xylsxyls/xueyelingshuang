#include "ListWidget.h"
#include "QssString.h"
#include "CStringManager.h"
#include <Windows.h>
#include <QEvent>
#include <QtGui/QPainter>
#include <qglobal.h>
#include <qmath.h>
#include "QssHelper.h"

ListWidget::ListWidget(QWidget* parent) :
QListWidget(parent),
m_hasSetBackgroundColor(false),
m_hasSetBorderWidth(false),
m_hasSetBorderColor(false),
m_hasSetItemBorderColor(false),
m_hasSetItemBorderWidth(false),
m_hasSetItemBorderImg(false),
m_loadItemBorderImgSuccess(false),
m_hasSetItemHeight(false),
m_hasSetTextColor(false),
m_hasSetFontName(false),
m_hasSetFontSize(false),
m_hasSetTextOrigin(false)
{
	
}

void ListWidget::setBackgroundColor(const QColor& color, bool rePaint)
{
	m_hasSetBackgroundColor = true;
	m_backgroundColor = color;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setBorderWidth(int32_t width, bool rePaint)
{
	m_hasSetBorderWidth = true;
	m_borderWidth = width;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setBorderColor(const QColor& color, bool rePaint)
{
	m_hasSetBorderColor = true;
	m_borderColor = color;

	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemBorderColor(const QColor& normalColor,
									const QColor& hoverColor,
									const QColor& disabledColor,
									bool rePaint)
{
	m_hasSetItemBorderColor = true;

	m_itemBorderColorMap[LIST_NORMAL] = normalColor;
	m_itemBorderColorMap[LIST_HOVER] = hoverColor;
	m_itemBorderColorMap[LIST_DISABLED] = disabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemBorderWidth(int32_t width, bool rePaint)
{
	m_hasSetItemBorderWidth = true;
	m_itemBorderWidth = width;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemBorderImage(const QString& borderImgPath,
									int32_t borderImgStateCount,
									int32_t borderImgNormal,
									int32_t borderImgHover,
									int32_t borderImgDisabled,
									bool rePaint)
{
	if (borderImgNormal > borderImgStateCount ||
		borderImgHover > borderImgStateCount ||
		borderImgDisabled > borderImgStateCount)
	{
		return;
	}

	m_hasSetItemBorderImg = true;

	m_loadItemBorderImgSuccess = QssHelper::GetPicHeight(borderImgPath.toStdWString(), borderImgStateCount, m_vecItemBorderImgHeight);
	m_itemBorderImgStateCount = borderImgStateCount;

	m_itemBorderImgPath = borderImgPath.toStdWString();
	m_itemBorderImgMap[LIST_NORMAL] = borderImgNormal;
	m_itemBorderImgMap[LIST_HOVER] = borderImgHover;
	m_itemBorderImgMap[LIST_DISABLED] = borderImgDisabled;

	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemHeight(int32_t height, bool rePaint)
{
	m_hasSetItemHeight = true;
	m_itemHeight = height;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setTextColor(const QColor& normalColor,
							  const QColor& hoverColor,
							  const QColor& disabledColor,
							  bool rePaint)
{
	m_hasSetTextColor = true;

	m_itemTextColorMap[LIST_NORMAL] = normalColor;
	m_itemTextColorMap[LIST_HOVER] = hoverColor;
	m_itemTextColorMap[LIST_DISABLED] = disabledColor;

	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setFontFace(const QString& fontName, bool rePaint)
{
	m_hasSetFontName = true;
	m_fontName = fontName.toStdWString();
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setFontSize(int32_t fontSize, bool rePaint)
{
	m_hasSetFontSize = true;
	m_fontSize = fontSize;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setTextOrigin(int32_t origin, bool rePaint)
{
	m_hasSetTextOrigin = true;
	m_textOrigin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemLeftOrigin(int32_t origin, bool rePaint)
{
	m_hasSetItemLeftOrigin = true;
	m_itemLeftOrgin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemTopOrigin(int32_t origin, bool rePaint)
{
	m_hasSetItemTopOrigin = true;
	m_itemTopOrgin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemRightOrigin(int32_t origin, bool rePaint)
{
	m_hasSetItemRightOrigin = true;
	m_itemRightOrgin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::setItemBottomOrigin(int32_t origin, bool rePaint)
{
	m_hasSetItemBottomOrigin = true;
	m_itemBottomOrgin = origin;
	if (rePaint)
	{
		updateStyle();
	}
}

void ListWidget::repaint()
{
	updateStyle();
}

void ListWidget::updateStyle()
{
	QssString buttonStyle;
	std::wstring className = L"QListWidget";

	//背景颜色
	if (m_hasSetBackgroundColor)
	{
		buttonStyle[className].AddKeyValue(L"background-color", QssHelper::QColorToWString(m_backgroundColor));
	}

	//边框粗度
	if (m_hasSetBorderWidth)
	{
		buttonStyle[className].AddKeyValue(L"border", CStringManager::Format(L"%dpx solid", m_borderWidth));
	}

	//设置边框颜色
	if (m_hasSetBorderColor)
	{
		buttonStyle[className].AddKeyValue(L"border-color", QssHelper::QColorToWString(m_borderColor));
	}

	//设置节点边框颜色
	if (m_hasSetItemBorderColor)
	{
		for (int32_t index = LIST_NORMAL; index <= LIST_DISABLED; ++index)
		{
			buttonStyle[className][L"item"](index).AddKeyValue(L"border-color", QssHelper::QColorToWString(m_itemBorderColorMap[index]));
		}
	}

	//节点边框粗度
	if (m_hasSetItemBorderWidth)
	{
		buttonStyle[className][L"item"].AddKeyValue(L"border", CStringManager::Format(L"%dpx solid", m_itemBorderWidth));
	}

	//节点背景图片
	if (m_hasSetItemBorderImg && m_loadItemBorderImgSuccess)
	{
		for (int32_t itemIndex = LIST_NORMAL; itemIndex <= LIST_DISABLED; ++itemIndex)
		{
			std::wstring imageUrl = CStringManager::Format(L"url(%s) %d 0 %d 0 stretch stretch",
				m_itemBorderImgPath.c_str(),
				m_vecItemBorderImgHeight[qMax(m_itemBorderImgMap[itemIndex] - 1, 0)],
				m_vecItemBorderImgHeight[qMin(m_itemBorderImgStateCount - m_itemBorderImgMap[itemIndex], m_itemBorderImgStateCount - 1)]);
			buttonStyle[className][L"item"](itemIndex).AddKeyValue(L"border-image", imageUrl);
		}
	}

	//设置节点高度
	if (m_hasSetItemHeight)
	{
		buttonStyle[className][L"item"].AddKeyValue(L"height", CStringManager::Format(L"%dpx", m_itemHeight));
	}

	//文字颜色
	if (m_hasSetTextColor)
	{
		for (int32_t itemIndex = LIST_NORMAL; itemIndex <= LIST_DISABLED; ++itemIndex)
		{
			QColor& color = m_itemTextColorMap[itemIndex];
			buttonStyle[className][L"item"](itemIndex).AddKeyValue(L"color", QssHelper::QColorToWString(color));
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
		buttonStyle[className][L"item"].AddKeyValue(L"padding-left", CStringManager::Format(L"%dpx", m_textOrigin));
	}

	//节点距离窗口左边距
	if (m_hasSetItemLeftOrigin)
	{
		buttonStyle[className][L"item"].AddKeyValue(L"margin-left", CStringManager::Format(L"%dpx", m_itemLeftOrgin));
	}

	//节点距离窗口上边距
	if (m_hasSetItemTopOrigin)
	{
		buttonStyle[className][L"item"].AddKeyValue(L"margin-top", CStringManager::Format(L"%dpx", m_itemTopOrgin));
	}

	//节点距离窗口右边距
	if (m_hasSetItemRightOrigin)
	{
		buttonStyle[className][L"item"].AddKeyValue(L"margin-right", CStringManager::Format(L"%dpx", m_itemRightOrgin));
	}

	//节点距离窗口下边距
	if (m_hasSetItemBottomOrigin)
	{
		buttonStyle[className][L"item"].AddKeyValue(L"margin-bottom", CStringManager::Format(L"%dpx", m_itemBottomOrgin));
	}

	setStyleSheet(QString::fromStdWString(buttonStyle.toWString()));
}

void ListWidget::showEvent(QShowEvent* eve)
{
	updateStyle();
}