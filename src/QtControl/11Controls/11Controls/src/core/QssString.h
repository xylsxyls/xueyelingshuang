#pragma once
#include <map>
#include <string>
#include <qglobal.h>
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

/** ����QSSʹ�õ��࣬����ֻ�����ڲ�����
*/
class QssString
{
public:
	QssString();

public:
	
public:
	QssString& operator[](const std::wstring& name);
	QssString& operator()(qint32 flag, const std::wstring& name);
	/** �ж�ʽ��ӽڵ���
	@param [in] hasItemName �Ƿ��нڵ���
	@param [in] itemName �ڵ���
	*/
	QssString& operator()(bool hasItemName, const std::wstring& itemName);
	QssString& operator()(const std::wstring& state);
	QssString& operator()(qint32 enumFlag);
	void AddKeyValue(const std::wstring& key, const std::wstring& value);
	std::wstring toWString();

private:
	std::wstring m_keyString;
	std::map<std::wstring, std::map<std::wstring, std::wstring>> m_mapData;
	std::map<qint32, std::wstring> m_mapEnum;
};