#pragma once
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include "QtControls/DialogShow.h"
#include "ViewBaseController.h"

class CustomViewBase;
class QResizeEvent;

/** 业务自定义窗口壳子，统一管理关闭结果、内容区所有权和显示模式
*/
class DialogManagerAPI CustomDialog :
	public DialogShow,
	public ViewBaseController
{
public:
	/** 构造函数
	*/
	CustomDialog();

	/** 析构函数
	*/
	virtual ~CustomDialog();

public:
	/** 初始化自定义窗口
	@param [in] param 窗口参数
	@return 返回true表示初始化成功
	*/
	virtual bool initDialog(const DialogParam& param);

	/** 设置自定义内容区，CustomDialog接管view的生命周期
	@param [in] view 自定义内容区指针
	@return 返回true表示设置成功
	*/
	bool setView(CustomViewBase* view);

	/** 获取当前自定义内容区
	@return 返回自定义内容区指针
	*/
	CustomViewBase* view() const;

	/** 设置窗口展示模式，自定义窗口支持模态和普通非模态
	@param [in] showMode 展示模式
	*/
	void setShowMode(DialogShowMode showMode);

	/** 获取窗口展示模式
	@return 返回展示模式
	*/
	DialogShowMode showMode() const;

	/** 判断当前自定义窗口是否按模态方式显示
	@return 返回true表示使用exec显示
	*/
	bool isModalMode() const;

	/** 更新自定义内容区
	@param [in] param 窗口参数
	*/
	void updateCustomView(const DialogParam& param);

public:
	/** 内容区希望修改窗口结果但不关闭窗口
	@param [in] view 发出请求的内容区
	@param [in] result 窗口结果
	@param [in] userResult 用户自定义结果
	*/
	void viewResultChanged(ViewBase* view, DialogResult result, qint32 userResult);

	/** 内容区希望关闭窗口
	@param [in] view 发出请求的内容区
	@param [in] result 窗口结果
	@param [in] userResult 用户自定义结果
	*/
	void viewCloseRequested(ViewBase* view, DialogResult result, qint32 userResult);

	/** 内容区希望调整窗口尺寸
	@param [in] view 发出请求的内容区
	@param [in] size 期望窗口尺寸
	*/
	void viewResizeRequested(ViewBase* view, const QSize& size);

protected:
	/** 窗口尺寸变化时同步内容区尺寸
	@param [in] eve Qt尺寸变化事件
	*/
	void resizeEvent(QResizeEvent* eve);

private:
	/** 禁止拷贝构造，避免Qt窗口对象被复制
	@param [in] other 另一个自定义窗口对象
	*/
	CustomDialog(const CustomDialog& other);

	/** 禁止赋值，避免Qt窗口对象被复制
	@param [in] other 另一个自定义窗口对象
	@return 返回当前对象引用
	*/
	CustomDialog& operator=(const CustomDialog& other);

private:
	// 自定义内容区，生命周期由CustomDialog管理
	CustomViewBase* m_view;
	// 当前自定义窗口展示模式
	DialogShowMode m_showMode;
};