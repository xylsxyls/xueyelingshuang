#include "QssString.h"

QssString::QssString()
{
	m_mapEnum[UNCHECK] = L"unchecked";
	m_mapEnum[CHECK] = L"checked";
	m_mapEnum[NORMAL] = L"";
	m_mapEnum[HOVER] = L"hover";
	m_mapEnum[SELECTED] = L"selected";
	m_mapEnum[PRESSED] = L"pressed";
	m_mapEnum[DISABLED] = L"disabled";
}

QssString& QssString::operator[](const std::wstring& name)
{
	if (m_keyString.empty())
	{
		m_keyString.append(L".");
		m_keyString.append(name);
	}
	else
	{
		m_keyString.append(L"::");
		m_keyString.append(name);
	}
	return *this;
}



QssString& QssString::operator()(int32_t flag, const std::wstring& name)
{
	m_keyString.append(L" ");
	m_keyString.append(name);
	return *this;
}

QssString& QssString::operator()(bool hasItemName, const std::wstring& itemName)
{
	if (hasItemName)
	{
		m_keyString.append(L"::");
		m_keyString.append(itemName);
	}
	return *this;
}

QssString& QssString::operator()(const std::wstring& state)
{
	m_keyString.append(L":");
	m_keyString.append(state);
	return *this;
}

QssString& QssString::operator()(int32_t enumFlag)
{
	std::wstring& str = m_mapEnum[enumFlag];
	if (str.empty() == false)
	{
		m_keyString.append(L":");
		m_keyString.append(str);
	}
	return *this;
}

void QssString::AddKeyValue(const std::wstring& key, const std::wstring& value)
{
	m_mapData[m_keyString][key] = value;
	m_keyString.clear();
}

std::wstring QssString::toWString()
{
	std::wstring result;
	for (auto itmapData = m_mapData.begin(); itmapData != m_mapData.end(); ++itmapData)
	{
		result.append(itmapData->first);
		result.append(L"{");
		auto& mapItem = itmapData->second;
		for (auto itmapItem = mapItem.begin(); itmapItem != mapItem.end(); ++itmapItem)
		{
			result.append(itmapItem->first);
			result.append(L":");
			result.append(itmapItem->second);
			result.append(L";");
		}
		result.append(L"}");
	}
	return result;
}