#pragma once
#include "QtControls/DialogResult.h"
#include <QSize>
#include <QtGlobal>
#include <stdint.h>

class ViewBase;

/** View回调控制器，由窗口壳子实现，用于接收内容区发出的结果和关闭请求
*/
class ViewBaseController
{
public:
	/** 析构函数
	*/
	virtual ~ViewBaseController();

public:
	/** 内容区希望修改窗口结果但不关闭窗口
	@param [in] view 发出请求的内容区
	@param [in] result 窗口结果
	@param [in] userResult 用户自定义结果
	*/
	virtual void viewResultChanged(ViewBase* view, DialogResult result, qint32 userResult) = 0;

	/** 内容区希望关闭窗口
	@param [in] view 发出请求的内容区
	@param [in] result 窗口结果
	@param [in] userResult 用户自定义结果
	*/
	virtual void viewCloseRequested(ViewBase* view, DialogResult result, qint32 userResult) = 0;

	/** 内容区希望调整窗口尺寸
	@param [in] view 发出请求的内容区
	@param [in] size 期望窗口尺寸
	*/
	virtual void viewResizeRequested(ViewBase* view, const QSize& size) = 0;
};