#pragma once
#include <map>
#include <string>
#include <qglobal.h>
#include "CoreMacro.h"
#include <vector>

enum
{
	UNCHECK,
	CHECK,
	NORMAL,
	HOVER,
	SELECTED,
	PRESSED,
	DISABLED,
	SPACE
};

struct KeyString
{
	std::wstring m_keyStr;
	std::wstring* m_keyPtr;
	KeyString(const std::wstring& keyStr):
		m_keyStr(keyStr),
		m_keyPtr(nullptr){}
	KeyString(std::wstring* keyPtr):
		m_keyPtr(keyPtr){}
};

struct Key
{
	std::vector<KeyString> m_vecKeyString;
};

/** 设置QSS使用的类，该类只用于内部调用
*/
class QssString
{
public:
	QssString();

public:
	void setClassName(const std::wstring& className);
	QssString& addClassName();
	QssString& operator()(qint32 enumFlag, const std::wstring& itemClassName);
	QssString& operator()(const std::wstring& str, std::wstring* name);
	/** 判断式添加节点名
	@param [in] hasItemName 是否有节点名
	@param [in] itemName 节点名
	*/
	QssString& operator()(bool hasItemName, const std::wstring& itemName);
	QssString& operator()(const std::wstring& state);
	QssString& operator()(qint32 enumFlag);
	void AddKeyValue(const std::wstring& key, const std::wstring& value);
	std::wstring toWString();
	
private:
	Key m_key;
	std::map<Key, std::map<std::wstring, std::wstring>> m_mapData;
	std::map<qint32, std::wstring> m_mapEnum;
	std::wstring m_className;
};