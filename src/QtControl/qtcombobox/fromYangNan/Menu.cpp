#include "Menu.h"
#include <QtWidgets/QWidgetAction>
#include "CustomStyle.h"
#include <qglobal.h>
#include <QMouseEvent>

Menu::Menu(QWidget* parent) :
ControlBase(parent),
m_pCustomStyle(nullptr)
{
	ControlBase::init(L"QMenu", L"item");
	m_pCustomStyle = new CustomStyle;
	if (m_pCustomStyle != nullptr)
	{
		setStyle(m_pCustomStyle);
	}
	installEventFilter(this);
	setMouseTracking(true);
}

Menu::~Menu()
{

}

Menu::Menu(const QString& title, const QString& icon, const QSize& size, QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QMenu", L"item");
	QMenu::setTitle(title);

	QMenu::setIcon(QPixmap(icon).scaled(size));
	m_pCustomStyle = new CustomStyle;
	if (m_pCustomStyle != nullptr)
	{
		setStyle(m_pCustomStyle);
	}
	installEventFilter(this);
	setMouseTracking(true);
}

QAction* Menu::addAction(const QString& text,
					 const QString& uncheckIcon,
					 const QSize& uncheckIconSize,
					 const QString& checkIcon,
					 const QSize& checkIconSize)
{
	if (m_pCustomStyle)
	{
		int32_t uncheckMax = qMax(uncheckIconSize.width(), uncheckIconSize.height());
		int32_t checkMax = qMax(checkIconSize.width(), checkIconSize.height());
		m_pCustomStyle->setMaxSize(qMax(uncheckMax, checkMax));
	}
	QAction* item = new QAction(this);
	item->setIcon(QPixmap(uncheckIcon).scaled(uncheckIconSize));
	item->setText(text);
	if (item == nullptr)
	{
		return nullptr;
	}
	item->installEventFilter(this);
	
	m_mapIconData[item] = QPixmap(checkIcon).scaled(checkIconSize);
	QMenu::addAction(item);
	return item;
}

void Menu::addMenu(Menu* menu)
{
	QMenu::addMenu(menu);
	m_vecMenu.push_back(menu);
}

void Menu::setSeparatorHeight(int32_t height, bool rePaint)
{
	m_itemName = L"separator";
	ControlBase::setPxValue(L"height", height, true, rePaint);
	m_itemName = L"item";
}

void Menu::setSeparatorColor(const QColor& color, bool rePaint)
{
	m_itemName = L"separator";
	std::map<int32_t, std::map<int32_t, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
	m_itemName = L"item";
}

void Menu::setItemIconOrigin(int32_t leftOrigin, bool rePaint)
{
	m_itemName = L"icon";
	ControlBase::setPxValue(L"padding-left", leftOrigin, true, rePaint);
	m_itemName = L"item";
}

QAction* Menu::exec(const QPoint& point)
{
	QAction* action = QMenu::exec(point);
	if (action == nullptr)
	{
		return action;
	}
	QIcon icon = action->icon();
	action->setIcon(ChangeIcon(action, icon));
	return action;
}

QIcon Menu::ChangeIcon(QAction* action, const QIcon& icon)
{
	//先寻找是否在本身节点中
	auto itIcon = m_mapIconData.find(action);
	if (itIcon != m_mapIconData.end())
	{
		QIcon result = itIcon->second;
		m_mapIconData[action] = icon;
		return result;
	}
	//如果本节点没有就找子节点
	int32_t index = -1;
	while (index++ != m_vecMenu.size() - 1)
	{
		Menu* menu = m_vecMenu[index];
		if (menu != nullptr)
		{
			return menu->ChangeIcon(action, icon);
		}
	}
	return QIcon();
}

//m_controlStyle[m_className](true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"top", L"1px");
//m_controlStyle[m_className](true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"left", L"1px");
//m_controlStyle[m_className](true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"right", L"1px");
//m_controlStyle[m_className](true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"bottom", L"1px");