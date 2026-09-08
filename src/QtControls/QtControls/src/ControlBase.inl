#ifndef CONTROLBASE_INL__
#define CONTROLBASE_INL__

#include "ControlBase.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ControlSubStyle.h"
#include "ControlShow.h"

template<class QBase>
ControlBase<QBase>::ControlBase():
m_show(nullptr)
{

}

template<class QBase>
ControlBase<QBase>::~ControlBase()
{

}

template<class QBase>
void ControlBase<QBase>::setControlShow(ControlShow<QBase>* show)
{
	m_show = show;
}

template<class QBase>
void ControlBase<QBase>::setFontFace(const std::wstring& fontName, bool isItem, bool rePaint)
{
	setKeyValue(L"font-family", CStringManager::Format(L"'%s'", fontName.c_str()), isItem, rePaint);
}

template<class QBase>
void ControlBase<QBase>::setPxValue(const std::wstring& keyWord, qint32 valuePx, bool isItem, bool rePaint)
{
	setKeyValue(keyWord, CStringManager::Format(L"%dpx", valuePx), isItem, rePaint);
}

template<class QBase>
void ControlBase<QBase>::setPxSolidValue(const std::wstring& keyWord, qint32 valuePxSolid, bool isItem, bool rePaint)
{
	setKeyValue(keyWord, CStringManager::Format(L"%dpx solid", valuePxSolid), isItem, rePaint);
}

template<class QBase>
void ControlBase<QBase>::setKeyValue(const std::wstring& keyWord,
									 const std::wstring& value,
									 bool isItem,
									 bool rePaint)
{
	if (m_show == nullptr)
	{
		return;
	}
	if (isItem)
	{
		ControlSubStyle::setKeyValue(&m_show->m_controlStyle, m_show->m_itemName, keyWord, value);
	}
	else
	{
		ControlSubStyle::setWidgetKeyValue(&m_show->m_controlStyle, keyWord, value);
	}
	if (rePaint)
	{
		m_show->repaint();
	}
}

template<class QBase>
void ControlBase<QBase>::setColorStateMap(const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
										  const std::wstring& keyWord,
										  bool isItem,
										  bool rePaint)
{
	if (m_show == nullptr)
	{
		return;
	}
	if (isItem)
	{
		ControlSubStyle::setColorStateMap(&m_show->m_controlStyle, m_show->m_itemName, colorStateMap, keyWord);
	}
	else
	{
		ControlSubStyle::setWidgetColorStateMap(&m_show->m_controlStyle, colorStateMap, keyWord);
	}
	if (rePaint)
	{
		m_show->repaint();
	}
}

template<class QBase>
void ControlBase<QBase>::setImageStateMap(const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
										  const std::wstring& imagePath,
										  qint32 stateCount,
										  const std::wstring& keyWord,
										  bool isItem,
										  bool rePaint)
{
	if (m_show == nullptr)
	{
		return;
	}
	if (isItem)
	{
		ControlSubStyle::setImageStateMap(&m_show->m_controlStyle, m_show->m_itemName, imagePath, stateCount, imageStateMap, keyWord);
	}
	else
	{
		ControlSubStyle::setWidgetImageStateMap(&m_show->m_controlStyle, imagePath, stateCount, imageStateMap, keyWord);
	}
	if (rePaint)
	{
		m_show->repaint();
	}
}

template<class QBase>
void ControlBase<QBase>::setImageStateMap(const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
										  const std::wstring& imagePath,
										  qint32 stateCount,
										  const std::wstring& keyWord,
										  const std::wstring& itemName,
										  bool rePaint)
{
	if (m_show == nullptr)
	{
		return;
	}
	ControlSubStyle::setImageStateMap(&m_show->m_controlStyle, itemName, imagePath, stateCount, imageStateMap, keyWord);
	if (rePaint)
	{
		m_show->repaint();
	}
}

#endif