#pragma once
#include "../core/QssString.h"
#include <QColor>
#include "ControlsMacro.h"

class QShowEvent;
class QWidget;
template <class QBase>
class ControlShow;
/** 控件通用设置相关功能
*/
template <class QBase>
class ControlBase
{
protected:
	/** 析构函数
	*/
	virtual ~ControlBase();

protected:
	/** 设置显示类指针
	@param [in] show 显示类指针
	*/
	void setControlShow(ControlShow<QBase>* show);

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
	void setPxValue(const std::wstring& keyWord, qint32 valuePx, bool isItem, bool rePaint);

	/** 设置线条对应值加solid
	@param [in] keyWord 关键词
	@param [in] valuePx 整形值，后面会加上px
	@param [in] isItem 该项是否为节点
	@param [in] rePaint 是否立即重画
	*/
	void setPxSolidValue(const std::wstring& keyWord, qint32 valuePxSolid, bool isItem, bool rePaint);

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
	void setColorStateMap(const std::map<qint32, std::map<qint32, QColor>>& colorStateMap,
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
	void setImageStateMap(const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
						  const std::wstring& imagePath,
						  qint32 stateCount,
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
	void setImageStateMap(const std::map<qint32, std::map<qint32, qint32>>& imageStateMap,
						  const std::wstring& imagePath,
						  qint32 stateCount,
						  const std::wstring& keyWord,
						  const std::wstring& itemName,
						  bool rePaint);

private:
	ControlShow<QBase>* m_show;
};

#include "ControlBase.inl"
