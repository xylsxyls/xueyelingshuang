#pragma once
#include <map>
#include <string>
#include <stdint.h>

enum
{
	UNCHECK,
	CHECK,
	NORMAL,
	HOVER,
	PRESSED,
	DISABLED,
	LIST_NORMAL,
	LIST_HOVER,
	LIST_DISABLED
};

class QssString
{
public:
	QssString();

public:
	
public:
	QssString& operator[](const std::wstring& name);
	QssString& operator()(const std::wstring& flag, const std::wstring& name);
	QssString& operator()(const std::wstring& state);
	QssString& operator()(int32_t enumFlag);
	void AddKeyValue(const std::wstring& key, const std::wstring& value);
	std::wstring toWString();

private:
	std::wstring m_keyString;
	std::map<std::wstring, std::map<std::wstring, std::wstring>> m_mapData;
	std::map<int32_t, std::wstring> m_mapEnum;
};