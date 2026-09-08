#pragma once
#include <map>
#include <string>
#include <qglobal.h>
#include <vector>
#include "QssStringMacro.h"

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
	/** 选择器片段文本，保存值本身，避免外部字符串变化破坏map排序
	*/
	std::wstring m_keyStr;

	/** 构造函数
	*/
	KeyString() :
	m_keyStr(){}

	/** 构造函数
	@param [in] keyStr 需要保存到选择器片段中的字符串
	*/
	KeyString(const std::wstring& keyStr):
		m_keyStr(keyStr){}
};

struct Key
{
	/** 按片段保存完整选择器，比较时逐片段比较，避免简单拼接造成key冲突
	*/
	std::vector<KeyString> m_vecKeyString;
};

/** 比较两个QSS选择器key
@param [in] key1 左侧key
@param [in] key2 右侧key
@return 返回true表示key1排序在key2之前
*/
bool operator<(const Key& key1, const Key& key2);

/** 设置QSS使用的类，该类只用于内部调用
*/
class QssStringAPI QssString
{
public:
	QssString();

public:
	/** 设置当前样式所属的控件类名
	@param [in] className 控件类名
	*/
	void setClassName(const std::wstring& className);

	/** 向当前选择器加入控件类名
	@return 返回当前对象，便于链式设置
	*/
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

	/** 写入当前选择器下的一条QSS键值并清空临时选择器
	@param [in] key QSS属性名
	@param [in] value QSS属性值
	*/
	void AddKeyValue(const std::wstring& key, const std::wstring& value);

	/** 生成完整QSS文本
	@return 返回可以直接设置给Qt控件的QSS字符串
	*/
	std::wstring toWString();
	
private:
	/** 当前正在拼接的选择器key
	*/
	Key m_key;

	/** 选择器到属性集合的映射
	*/
	std::map<Key, std::map<std::wstring, std::wstring>> m_mapData;

	/** 内置状态枚举到QSS伪状态文本的映射
	*/
	std::map<qint32, std::wstring> m_mapEnum;

	/** 当前控件类名，由QssHelper按真实控件类型写入
	*/
	std::wstring m_className;
};
