#include "ComboBox.h"
#include <stdint.h>
#include "QssString.h"
#include "CStringManager.h"
#include <qglobal.h>
#include "QssHelper.h"
#include "NoFocusFrameDelegate.h"
#include "ListWidget.h"

ComboBox::ComboBox(QWidget* parent) :
ControlBase(parent)
{
	init(L"QComboBox", L"drop-down");
	//下拉边框粗度设为0，因为QListWidget已有边框，此属性px无效
	m_controlStyle[m_className](1, L"QAbstractItemView").AddKeyValue(L"border", L"none");
	m_listWidget = new ListWidget;
	setModel(m_listWidget->model());
	setView(m_listWidget);
	setItemDelegate(new NoFocusFrameDelegate);
}

void ComboBox::setBorderRadius(int32_t radius, bool rePaint)
{
	ControlBase::setPxValue(L"border-radius", radius, false, rePaint);
}

void ComboBox::setBackgroundColor(const QColor& normalColor,
								  const QColor& hoverColor,
								  const QColor& pressedColor,
								  const QColor& disabledColor,
								  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][PRESSED] = pressedColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlBase::setColorStateMap(colorStateMap, L"background-color", false, rePaint);
}

void ComboBox::setBorderWidth(int32_t width, bool rePaint)
{
	ControlBase::setPxSolidValue(L"border", width, false, rePaint);
}

void ComboBox::setBorderColor(const QColor& normalColor,
						      const QColor& hoverColor,
							  const QColor& pressedColor,
							  const QColor& disabledColor,
							  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][PRESSED] = pressedColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlBase::setColorStateMap(colorStateMap, L"border-color", false, rePaint);
}

void ComboBox::setBorderImage(const QString& borderImgPath,
							  int32_t borderImgStateCount,
							  int32_t borderImgNormal,
							  int32_t borderImgHover,
							  int32_t borderImgPressed,
							  int32_t borderImgDisabled,
							  bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = borderImgNormal;
	imageStateMap[NORMAL][HOVER] = borderImgHover;
	imageStateMap[NORMAL][PRESSED] = borderImgPressed;
	imageStateMap[NORMAL][DISABLED] = borderImgDisabled;
	std::wstring wstrImgPath = borderImgPath.toStdWString();
	ControlBase::setImageStateMap(imageStateMap, wstrImgPath, borderImgStateCount, L"border-image", false, rePaint);
}

void ComboBox::setDropDownSize(int32_t width, int32_t height, bool rePaint)
{
	ControlBase::setPxValue(L"width", width, true, false);
	ControlBase::setPxValue(L"height", width, true, rePaint);
}

void ComboBox::setDropDownBorderWidth(int32_t width, bool rePaint)
{
	ControlBase::setPxSolidValue(L"border", width, true, rePaint);
}

void ComboBox::setDropDownImage(const QString& dropDownImgPath, bool rePaint)
{
	std::map<int32_t, std::map<int32_t, int32_t>> imageStateMap;
	imageStateMap[NORMAL][NORMAL] = 1;
	std::wstring wstrImgPath = dropDownImgPath.toStdWString();
	ControlBase::setImageStateMap(imageStateMap, wstrImgPath, 1, L"image", L"down-arrow", rePaint);
}

void ComboBox::setTextColor(const QColor& normalColor,
							const QColor& hoverColor,
							const QColor& pressedColor,
							const QColor& disabledColor,
							bool rePaint)
{
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = normalColor;
	colorStateMap[NORMAL][HOVER] = hoverColor;
	colorStateMap[NORMAL][PRESSED] = pressedColor;
	colorStateMap[NORMAL][DISABLED] = disabledColor;
	ControlBase::setColorStateMap(colorStateMap, L"color", false, rePaint);
}

void ComboBox::setFontFace(const QString& fontName, bool rePaint)
{
	ControlBase::setFontFace(fontName.toStdWString(), false, rePaint);
}

void ComboBox::setFontSize(int32_t fontSize, bool rePaint)
{
	ControlBase::setPxValue(L"font-size", fontSize, false, rePaint);
}

void ComboBox::setTextOrigin(int32_t origin, bool rePaint)
{
	ControlBase::setPxValue(L"padding-left", origin, false, rePaint);
}

void ComboBox::setListOrigin(int32_t origin, bool rePaint)
{
	std::wstring wstrOrigin = CStringManager::Format(L"%dpx", origin);
	m_controlStyle[m_className](1, L"QAbstractItemView").AddKeyValue(L"margin-top", wstrOrigin);
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

void ComboBox::setListBackgroundColor(const QColor& color, bool rePaint)
{
	m_listWidget->setBackgroundColor(color, rePaint);
}

void ComboBox::setListBorderWidth(int32_t width, bool rePaint)
{
	m_listWidget->setBorderWidth(width, rePaint);
}

void ComboBox::setListBorderColor(const QColor& color, bool rePaint)
{
	m_listWidget->setBorderColor(color, rePaint);
}

void ComboBox::setListItemBorderColor(const QColor& normalColor,
									const QColor& hoverColor,
									const QColor& disabledColor,
									bool rePaint)
{
	m_listWidget->setItemBorderColor(normalColor, hoverColor, disabledColor, rePaint);
}

void ComboBox::setListItemBorderWidth(int32_t width, bool rePaint)
{
	m_listWidget->setItemBorderWidth(width, rePaint);
}

void ComboBox::setListItemBorderImage(const QString& borderImgPath,
									int32_t borderImgStateCount,
									int32_t borderImgNormal,
									int32_t borderImgHover,
									int32_t borderImgDisabled,
									bool rePaint)
{
	m_listWidget->setItemBorderImage(borderImgPath, borderImgStateCount, borderImgNormal, borderImgHover, borderImgDisabled, rePaint);
}

void ComboBox::setListItemHeight(int32_t height, bool rePaint)
{
	m_listWidget->setItemHeight(height, rePaint);
}

void ComboBox::setListTextColor(const QColor& normalColor,
							  const QColor& hoverColor,
							  const QColor& disabledColor,
							  bool rePaint)
{
	m_listWidget->setTextColor(normalColor, hoverColor, disabledColor, rePaint);
}

void ComboBox::setListFontFace(const QString& fontName, bool rePaint)
{
	m_listWidget->setFontFace(fontName, rePaint);
}

void ComboBox::setListFontSize(int32_t fontSize, bool rePaint)
{
	m_listWidget->setFontSize(fontSize, rePaint);
}

void ComboBox::setListTextOrigin(int32_t origin, bool rePaint)
{
	m_listWidget->setTextOrigin(origin, rePaint);
}

void ComboBox::setListItemOrigin(int32_t leftOrigin,
								 int32_t topOrigin,
								 int32_t rightOrigin,
								 int32_t bottomOrigin,
								 bool rePaint)
{
	m_listWidget->setItemOrigin(leftOrigin, topOrigin, rightOrigin, bottomOrigin, rePaint);
}

void ComboBox::showEvent(QShowEvent* eve)
{
	repaint();
}

void ComboBox::repaint()
{
	updateStyle();
	m_listWidget->repaint();
}