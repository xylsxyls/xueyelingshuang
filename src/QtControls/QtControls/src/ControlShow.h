#pragma once
#include "QssString/QssStringAPI.h"
#include <string>
#include <QColor>
#include "QtControlsMacro.h"

class QShowEvent;
class QWidget;
/** 控件通用样式模板
*/
template <class QBase>
class ControlShow : public QBase
{
public:
	/** 重画
	*/
	virtual void repaint();

protected:
	/** 构造函数
	@param [in] parent 父窗口指针
	*/
	ControlShow(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ControlShow();

public:
	/** 初始化类名
	*/
	void initClassName();

	/** 设置子节点名
	@param [in] itemName 子节点名
	*/
	void setItemName(const std::wstring& itemName);

protected:
	/** 获取类名
	@return 返回类名
	*/
	std::wstring className();

protected:
	/** 首次显示时初始化类名并刷新样式
	@param [in] eve Qt显示事件
	*/
	virtual void showEvent(QShowEvent* eve);

public:
	// 控件QSS字符串管理对象
	QssString m_controlStyle;
	// 子节点QSS名称
	std::wstring m_itemName;
};

#include "ControlShow.inl"