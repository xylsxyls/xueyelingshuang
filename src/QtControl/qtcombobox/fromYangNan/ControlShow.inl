#ifndef CONTROLSHOW_INL__
#define CONTROLSHOW_INL__

#include "ControlShow.h"
#include "QssString.h"
#include "CStringManager.h"
#include "QssHelper.h"
#include <QDebug>

template<class QBase>
void ControlShow<QBase>::repaint()
{
	QBase::setStyleSheet(QString::fromStdWString(m_controlStyle.toWString()));
	qDebug() << QString::fromStdWString(m_controlStyle.toWString());
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

template<class QBase>
bool ControlShow<QBase>::init(const std::wstring& className, const std::wstring& itemName)
{
	m_className = className;
	m_itemName = itemName;
	return true;
}

template<class QBase>
void ControlShow<QBase>::showEvent(QShowEvent*)
{
	repaint();
}

#endif