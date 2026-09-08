#include "Menu.h"
#include <QtWidgets/QWidgetAction>
#include "MenuCustomStyle.h"
#include "ControlSubStyle.h"
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
		m_pCustomStyle->setParent(this);
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
	m_pCustomStyle = nullptr;
}

Menu::Menu(const QString& title, const QString& icon, const QSize& size, QWidget* parent) :
ControlShow(parent),
m_pCustomStyle(nullptr)
{
	ControlBase::setControlShow(this);
	setItemName(L"item");
	QMenu::setTitle(title);

	QMenu::setIcon(QPixmap(icon).scaled(size));
	m_pCustomStyle = new MenuCustomStyle;
	if (m_pCustomStyle != nullptr)
	{
		m_pCustomStyle->setParent(this);
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
	if (item == nullptr)
	{
		return nullptr;
	}
	
	item->setText(text);
	item->installEventFilter(this);

	if (!uncheckIcon.isEmpty() || !checkIcon.isEmpty())
	{
		item->setIcon(QPixmap(uncheckIcon).scaled(uncheckIconSize));
		item->setCheckable(true);
		m_mapIconData[item] = QPixmap(checkIcon).scaled(checkIconSize);
	}
	QMenu::addAction(item);
	return item;
}

void Menu::addMenu(Menu* menu)
{
	if (menu == nullptr)
	{
		return;
	}
	QMenu::addMenu(menu);
	m_vecMenu.push_back(menu);
}

void Menu::setSeparatorHeight(qint32 height, bool rePaint)
{
	ControlSubStyle::setPxValue(&m_controlStyle, L"separator", L"height", qMax(height, 0));
	if (rePaint)
	{
		repaint();
	}
}

void Menu::setSeparatorColor(const QColor& color, bool rePaint)
{
	std::map<qint32, std::map<qint32, QColor>> colorStateMap;
	colorStateMap[NORMAL][NORMAL] = color;
	ControlSubStyle::setColorStateMap(&m_controlStyle, L"separator", colorStateMap, L"background-color");
	if (rePaint)
	{
		repaint();
	}
}

void Menu::setItemIconOrigin(qint32 leftOrigin, bool rePaint)
{
	ControlSubStyle::setPxValue(&m_controlStyle, L"icon", L"padding-left", qMax(leftOrigin, 0));
	if (rePaint)
	{
		repaint();
	}
}

QAction* Menu::exec(const QPoint& point)
{
	QAction* action = QMenu::exec(point);
	if (action == nullptr)
	{
		return nullptr;
	}
	QIcon icon = action->icon();
	QIcon changedIcon;
	if (tryChangeIcon(action, icon, &changedIcon))
	{
		action->setIcon(changedIcon);
	}
	return action;
}

bool Menu::tryChangeIcon(QAction* action, const QIcon& icon, QIcon* changedIcon)
{
	if (action == nullptr || changedIcon == nullptr)
	{
		return false;
	}
	//先寻找是否在本身节点中
	auto itIcon = m_mapIconData.find(action);
	if (itIcon != m_mapIconData.end())
	{
		*changedIcon = itIcon->second;
		m_mapIconData[action] = icon;
		return true;
	}
	//如果本节点没有就找子节点
	const qint32 count = static_cast<qint32>(m_vecMenu.size());
	for (qint32 index = 0; index < count; ++index)
	{
		Menu* menu = m_vecMenu[index];
		if (menu != nullptr)
		{
			if (menu->tryChangeIcon(action, icon, changedIcon))
			{
				return true;
			}
		}
	}
	return false;
}

//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"top", L"1px");
//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"left", L"1px");
//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"right", L"1px");
//m_controlStyle.addClassName()(true, m_itemName)(UNCHECK)(NORMAL).AddKeyValue(L"bottom", L"1px");