#pragma once
#include "DialogManagerMacro.h"
#include "DialogType.h"
#include <QSize>
#include <QWidget>

class ViewBaseController;

/** 弹窗内容区基类，只负责内部控件、布局、样式和用户交互，不直接管理窗口生命周期
*/
class DialogManagerAPI ViewBase : public QWidget
{
public:
	/** 构造函数
	@param [in] parent 父控件指针
	*/
	explicit ViewBase(QWidget* parent = nullptr);

	/** 析构函数
	*/
	virtual ~ViewBase();

public:
	/** 初始化内容区
	@param [in] param 窗口参数
	@return 返回true表示初始化成功
	*/
	virtual bool initView(const DialogParam& param);

	/** 获取内容区期望窗口尺寸
	@return 返回期望窗口尺寸，无效尺寸表示由外层窗口自行决定
	*/
	virtual QSize preferredSize() const;

	/** 获取内容区最小窗口尺寸
	@return 返回最小窗口尺寸，无效尺寸表示不额外限制
	*/
	virtual QSize minimumViewSize() const;

	/** 获取默认焦点控件
	@return 返回默认焦点控件，没有默认焦点时返回nullptr
	*/
	virtual QWidget* defaultFocusWidget() const;

	/** 更新内容区，用于复用同一个窗口时刷新显示内容
	@param [in] param 窗口参数
	*/
	virtual void updateView(const DialogParam& param);

	/** 设置内容区回调控制器
	@param [in] controller 回调控制器指针，ViewBase不负责释放
	*/
	void setController(ViewBaseController* controller);

protected:
	/** 通知外层窗口修改结果但不关闭窗口
	@param [in] result 窗口结果
	@param [in] userResult 用户自定义结果
	*/
	void notifyResultChanged(DialogResult result, qint32 userResult);

	/** 通知外层窗口关闭
	@param [in] result 窗口结果
	@param [in] userResult 用户自定义结果
	*/
	void notifyCloseRequested(DialogResult result, qint32 userResult);

	/** 通知外层窗口调整尺寸
	@param [in] size 期望窗口尺寸
	*/
	void notifyResizeRequested(const QSize& size);

private:
	/** 禁止拷贝构造，避免Qt控件对象被复制
	@param [in] other 另一个内容区对象
	*/
	ViewBase(const ViewBase& other);

	/** 禁止赋值，避免Qt控件对象被复制
	@param [in] other 另一个内容区对象
	@return 返回当前对象引用
	*/
	ViewBase& operator=(const ViewBase& other);

private:
	// 内容区回调控制器，不负责释放
	ViewBaseController* m_controller;
};