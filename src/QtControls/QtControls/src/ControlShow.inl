#ifndef CONTROLSHOW_INL__
#define CONTROLSHOW_INL__

#include "ControlShow.h"
#include "QtCore/QssString.h"
#include "core/CStringManager.h"
#include "QtCore/QssHelper.h"

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
	return QString::fromStdString(CStringManager::Replace(typeid(*this).name(), "class ", "")).toStdWString();
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