#pragma once
#include "ViewBase.h"

/** 业务自定义窗口内容区基类，业务只能通过该类扩展自定义窗口内容
*/
class DialogManagerAPI CustomViewBase : public ViewBase
{
public:
	/** 构造函数
	@param [in] parent 父控件指针
	*/
	explicit CustomViewBase(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~CustomViewBase();

private:
	/** 禁止拷贝构造，避免Qt控件对象被复制
	@param [in] other 另一个自定义内容区对象
	*/
	CustomViewBase(const CustomViewBase& other);

	/** 禁止赋值，避免Qt控件对象被复制
	@param [in] other 另一个自定义内容区对象
	@return 返回当前对象引用
	*/
	CustomViewBase& operator=(const CustomViewBase& other);
};