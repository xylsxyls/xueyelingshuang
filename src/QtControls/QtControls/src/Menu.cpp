#include "Menu.h"
#include <QtWidgets/QWidgetAction>
#include "MenuCustomStyle.h"
#include <qglobal.h>
#include <QMouseEvent>

Menu::Menu(QWidget* parent) :
ControlShow(parent),
m_pCustomStyle(nullptr)
{
	ControlBase::setControlShow(this);
	setItemName(L"item");
	m_pCustomStyle = new MenuCustomStyle;
	if (m_pCustomStyle != nullptr)
	{
		setStyle(m_pCustomStyle);
	}
	installEventFilter(this);
	setMouseTracking(true);

	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	//背景透明
	setAttribute(Qt::WA_TranslucentBackground);
}

Menu::~Menu()
{

}

Menu::Menu(const QString& title, const QString& icon, const QSize& size, QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setItemName(L"item");
	QMenu::setTitle(title);

	QMenu::setIcon(QPixmap(icon).scaled(size));
	m_pCustomStyle = new MenuCustomStyle;
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
	if (m_pCustomStyle != nullptr)
	{
		qint32 uncheckMax = qMax(uncheckIconSize.width(), uncheckIconSize.height());
		qint32 checkMax = qMax(checkIconSize.width(), checkIconSize.height());
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

void Menu::setSeparatorHeight(qint32 height, bool rePaint)
{
	m_itemName = L"separator";
	ControlBase::setPxValue(L"height", height, true, rePaint);
	m_itemName = L"item";
}

void Menu::setSeparatorColor(const QColor& color, bool rePaint)
{
	m_itemName = L"separator";
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlBase::setColorStateMap(colorStateMap, L"background-color", true, rePaint);
	m_itemName = L"item";
}

void Menu::setItemIconOrigin(qint32 leftOrigin, bool rePaint)
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
		return nullptr;
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
	qint32 index = -1;
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

//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"top", L"1px");
//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"left", L"1px");
//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"right", L"1px");
//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"bottom", L"1px");