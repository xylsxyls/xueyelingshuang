#pragma once
#include "../core/QssString.h"
#include <string>
#include <QColor>
#include "ControlsMacro.h"

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
	virtual void showEvent(QShowEvent* eve);

public:
	//style字符串管理对象
	QssString m_controlStyle;
	//节点名字符串
	std::wstring m_itemName;
};

#include "ControlShow.inl"
