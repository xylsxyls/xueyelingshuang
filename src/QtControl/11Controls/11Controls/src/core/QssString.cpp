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
	m_mapEnum[SPACE] = L" ";
}

bool operator < (const Key& key1, const Key& key2)
{
	std::wstring result1;
	for (auto itKey1 = key1.m_vecKeyString.begin(); itKey1 != key1.m_vecKeyString.end(); ++itKey1)
	{
		result1 += itKey1->m_keyStr;
	}
	std::wstring result2;
	for (auto itKey2 = key2.m_vecKeyString.begin(); itKey2 != key2.m_vecKeyString.end(); ++itKey2)
	{
		result2 += itKey2->m_keyStr;
	}

	return result1 < result2;
}

void QssString::setClassName(const std::wstring& className)
{
	m_className = className;
}

QssString& QssString::addClassName()
{
	return operator()(m_key.m_vecKeyString.empty() ? L"." : L"::", &m_className);
}

QssString& QssString::operator()(const std::wstring& str, std::wstring* name)
{
	m_key.m_vecKeyString.push_back(str);
	m_key.m_vecKeyString.push_back(name);
	return *this;
}

QssString& QssString::operator()(bool hasItemName, const std::wstring& itemName)
{
	if (hasItemName)
	{
		m_key.m_vecKeyString.push_back(KeyString(L"::"));
		m_key.m_vecKeyString.push_back(itemName);
	}
	return *this;
}

QssString& QssString::operator()(const std::wstring& state)
{
	m_key.m_vecKeyString.push_back(KeyString(L":"));
	m_key.m_vecKeyString.push_back(state);
	return *this;
}

QssString& QssString::operator()(qint32 enumFlag)
{
	std::wstring& str = m_mapEnum[enumFlag];
	if (str.empty() == false)
	{
		m_key.m_vecKeyString.push_back(KeyString(L":"));
		m_key.m_vecKeyString.push_back(str);
	}
	return *this;
}

QssString& QssString::operator()(qint32 enumFlag, const std::wstring& itemClassName)
{
	m_key.m_vecKeyString.push_back(m_mapEnum[enumFlag]);
	m_key.m_vecKeyString.push_back(itemClassName);
	return *this;
}

void QssString::AddKeyValue(const std::wstring& key, const std::wstring& value)
{
	m_mapData[m_key][key] = value;
	m_key.m_vecKeyString.clear();
}

std::wstring QssString::toWString()
{
	std::wstring result;
	for (auto itmapData = m_mapData.begin(); itmapData != m_mapData.end(); ++itmapData)
	{
		auto& vecKey = itmapData->first.m_vecKeyString;
		for (auto itKey = vecKey.begin(); itKey != vecKey.end(); ++itKey)
		{
			result.append(itKey->m_keyPtr == nullptr ? itKey->m_keyStr : *(itKey->m_keyPtr));
		}
		
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