#ifndef CONTROLBASE_INL__
#define CONTROLBASE_INL__

#include "ControlBase.h"
#include "QssString.h"
#include "CStringManager.h"
#include "QssHelper.h"
#include "ControlShow.h"

template<class QBase>
ControlBase<QBase>::~ControlBase()
{

}

template<class QBase>
void ControlBase<QBase>::setControlShow(ControlShow<QBase>* show)
{
	//�����ϲ��ó��ִ��յ����󣬵��������º�����û���пգ�����������ͳһ�ж�һ��
	if (show == nullptr)
	{
		abort();
	}
	m_show = show;
}

template<class QBase>
void ControlBase<QBase>::setFontFace(const std::wstring& fontName, bool isItem, bool rePaint)
{
	setKeyValue(L"font-family", CStringManager::Format(L"'%s'", fontName.c_str()), isItem, rePaint);
}

template<class QBase>
void ControlBase<QBase>::setPxValue(const std::wstring& keyWord, int32_t valuePx, bool isItem, bool rePaint)
{
	setKeyValue(keyWord, CStringManager::Format(L"%dpx", valuePx), isItem, rePaint);
}

template<class QBase>
void ControlBase<QBase>::setPxSolidValue(const std::wstring& keyWord, int32_t valuePxSolid, bool isItem, bool rePaint)
{
	setKeyValue(keyWord, CStringManager::Format(L"%dpx solid", valuePxSolid), isItem, rePaint);
}

template<class QBase>
void ControlBase<QBase>::setKeyValue(const std::wstring& keyWord,
									 const std::wstring& value,
									 bool isItem,
									 bool rePaint)
{
	m_show->m_controlStyle[m_show->m_className](isItem, m_show->m_itemName).AddKeyValue(keyWord, value);
	if (rePaint)
	{
		m_show->repaint();
	}
}

template<class QBase>
void ControlBase<QBase>::setColorStateMap(const std::map<int32_t, std::map<int32_t, QColor>>& colorStateMap,
										  const std::wstring& keyWord,
										  bool isItem,
										  bool rePaint)
{
	for (auto itStateMap = colorStateMap.begin(); itStateMap != colorStateMap.end(); ++itStateMap)
	{
		auto& state = itStateMap->first;
		auto& colorMap = itStateMap->second;
		for (auto itColor = colorMap.begin(); itColor != colorMap.end(); ++itColor)
		{
			auto& stateInMap = itColor->first;
			const QColor& color = itColor->second;
			std::wstring colorString = QssHelper::QColorToWString(color);
			m_show->m_controlStyle[m_show->m_className](isItem, m_show->m_itemName)(state)(stateInMap).AddKeyValue(keyWord, colorString);
		}
	}
	if (rePaint)
	{
		m_show->repaint();
	}
}

template<class QBase>
void ControlBase<QBase>::setImageStateMap(const std::map<int32_t, std::map<int32_t, int32_t>>& imageStateMap,
										  const std::wstring& imagePath,
										  int32_t stateCount,
										  const std::wstring& keyWord,
										  bool isItem,
										  bool rePaint)
{
	std::vector<int32_t> vecHeight;
	if (QssHelper::GetPicHeight(imagePath, stateCount, vecHeight) == false)
	{
		return;
	}

	for (auto itStateMap = imageStateMap.begin(); itStateMap != imageStateMap.end(); ++itStateMap)
	{
		auto& state = itStateMap->first;
		auto& imageMap = itStateMap->second;
		for (auto itImage = imageMap.begin(); itImage != imageMap.end(); ++itImage)
		{
			auto& stateInMap = itImage->first;
			const int32_t& imageNum = itImage->second;
			if (imageNum > stateCount || imageNum <= 0)
			{
				continue;
			}
			std::wstring imageUrl = CStringManager::Format(L"url(%s) %d 0 %d 0 stretch stretch",
														   imagePath.c_str(),
														   vecHeight[imageNum - 1],
														   vecHeight[stateCount - imageNum]);
			m_show->m_controlStyle[m_show->m_className](isItem, m_show->m_itemName)(state)(stateInMap).AddKeyValue(keyWord, imageUrl);
		}
	}
	if (rePaint)
	{
		m_show->repaint();
	}
}

template<class QBase>
void ControlBase<QBase>::setImageStateMap(const std::map<int32_t, std::map<int32_t, int32_t>>& imageStateMap,
										  const std::wstring& imagePath,
										  int32_t stateCount,
										  const std::wstring& keyWord,
										  const std::wstring& itemName,
										  bool rePaint)
{
	std::wstring itemNameBk = m_show->m_itemName;
	m_show->m_itemName = itemName;
	setImageStateMap(imageStateMap, imagePath, stateCount, keyWord, true, rePaint);
	m_show->m_itemName = itemNameBk;
}

#endif