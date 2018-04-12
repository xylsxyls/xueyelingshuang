#pragma once
#include <map>
#include <string>
#include <stdint.h>
#include "CoreMacro.h"

enum
{
	UNCHECK,
	CHECK,
	NORMAL,
	HOVER,
	SELECTED,
	PRESSED,
	DISABLED
};

/** 设置QSS使用的类，该类只用于内部调用
*/
class QssString
{
public:
	QssString();

public:
	
public:
	QssString& operator[](const std::wstring& name);
	QssString& operator()(int32_t flag, const std::wstring& name);
	/** 判断式添加节点名
	@param [in] hasItemName 是否有节点名
	@param [in] itemName 节点名
	*/
	QssString& operator()(bool hasItemName, const std::wstring& itemName);
	QssString& operator()(const std::wstring& state);
	QssString& operator()(int32_t enumFlag);
	void AddKeyValue(const std::wstring& key, const std::wstring& value);
	std::wstring toWString();

private:
	std::wstring m_keyString;
	std::map<std::wstring, std::map<std::wstring, std::wstring>> m_mapData;
	std::map<int32_t, std::wstring> m_mapEnum;
};