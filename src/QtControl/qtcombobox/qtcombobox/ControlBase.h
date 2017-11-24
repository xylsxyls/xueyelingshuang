#ifndef CONTROLBASE_H__
#define CONTROLBASE_H__
//#pragma once
#include "QssString.h"
#include <string>
#include <QColor>

class QShowEvent;
class QWidget;
template <class QBase>
class ControlBase : public QBase
{
public:
	/** 重画
	*/
	virtual void repaint();

protected:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ControlBase(QWidget* parent = NULL);

	virtual ~ControlBase(){};

	/** 初始化
	@param [in] className 类名
	@param [in] itemName 节点名
	*/
	void init(const std::wstring& className, const std::wstring& itemName);

	/** 设置字体
	@param [in] fontName 字体名
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setFontFace(const std::wstring& fontName, bool isItem, bool rePaint);

	/** 设置整形对应值
	@param [in] keyWord 关键词
	@param [in] valuePx 整形值，后面会加上px
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setPxValue(const std::wstring& keyWord, int32_t valuePx, bool isItem, bool rePaint);

	/** 设置线条对应值加solid
	@param [in] keyWord 关键词
	@param [in] valuePx 整形值，后面会加上px
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setPxSolidValue(const std::wstring& keyWord, int32_t valuePxSolid, bool isItem, bool rePaint);

	/** 设置对应值
	@param [in] keyWord 关键词
	@param [in] value 值
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setKeyValue(const std::wstring& keyWord, const std::wstring& value, bool isItem, bool rePaint);

	/** 设置颜色
	@param [in] colorStateMap 颜色集合
	@param [in] keyWord 关键词
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setColorStateMap(const std::map<int32_t, std::map<int32_t, QColor>>& colorStateMap,
						  const std::wstring& keyWord,
						  bool isItem,
						  bool rePaint);

	/** 设置图片
	@param [in] imageStateMap 图片集合
	@param [in] imagePath 路径
	@param [in] stateCount 状态个数
	@param [in] keyWord 关键词
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setImageStateMap(const std::map<int32_t, std::map<int32_t, int32_t>>& imageStateMap,
						  const std::wstring& imagePath,
						  int32_t stateCount,
						  const std::wstring& keyWord,
						  bool isItem,
						  bool rePaint);

	/** 设置图片
	@param [in] imageStateMap 图片集合
	@param [in] imagePath 路径
	@param [in] stateCount 状态个数
	@param [in] keyWord 关键词
	@param [in] itemName 节点名
	@param [in] rePaint 是否立即重画
	*/
	void setImageStateMap(const std::map<int32_t, std::map<int32_t, int32_t>>& imageStateMap,
						  const std::wstring& imagePath,
						  int32_t stateCount,
						  const std::wstring& keyWord,
						  const std::wstring& itemName,
						  bool rePaint);

protected:
	virtual void updateStyle();
	virtual void showEvent(QShowEvent* eve);

protected:
	QssString m_controlStyle;
	std::wstring m_className;
	std::wstring m_itemName;
};

#include "ControlBase.inl"

#endif