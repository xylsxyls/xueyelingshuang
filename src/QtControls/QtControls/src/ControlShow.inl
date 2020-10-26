#ifndef CONTROLSHOW_INL__
#define CONTROLSHOW_INL__

#include "ControlShow.h"
#include "CStringManager/CStringManagerAPI.h"

template<class QBase>
void ControlShow<QBase>::repaint()
{
	initClassName();
	QBase::setStyleSheet(QString::fromStdWString(m_controlStyle.toWString()));
}

template<class QBase>
ControlShow<QBase>::ControlShow(QWidget* parent) :
QBase(parent)
{
	
}

template<class QBase>
ControlShow<QBase>::~ControlShow()
{

}

template <class QBase>
std::wstring ControlShow<QBase>::className()
{
	std::string className = typeid(*this).name();
#ifdef _MSC_VER
	CStringManager::Replace(className, "class ", "");
#elif __linux__
	for (auto it = className.begin(); it != className.end();)
	{
		if (*it >= 48 && *it <= 57)
		{
			it = className.erase(it);
			continue;
		}
		break;
	}
#endif
	return QString::fromStdString(className).toStdWString();
}

template <class QBase>
void ControlShow<QBase>::initClassName()
{
	m_controlStyle.setClassName(className());
}

template <class QBase>
void ControlShow<QBase>::setItemName(const std::wstring& itemName)
{
	m_itemName = itemName;
}

template<class QBase>
void ControlShow<QBase>::showEvent(QShowEvent* eve)
{
	repaint();
	QBase::showEvent(eve);
}

#endif