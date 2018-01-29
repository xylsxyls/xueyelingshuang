#pragma once
#include <QDialog>
#include <stdint.h>

class Label;
class COriginalButton;
class QSplitter;
class LineEdit;
class CPasswordInputBox;
/** 弹出框基类，实现基础接口
*/
class DialogBase : public QDialog
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	DialogBase();

	/** 展示窗口
	@param [out] dialogId 窗口ID值
	@param [in] timeOut 自动关闭时间，单位秒，如果不填则表示不会自动关闭
	@param [in] isCountDownVisible 是否显示倒计时
	@return 关闭窗口时给的返回值
	*/
	int32_t exec(int32_t& dialogId, int32_t timeOut = -1, bool isCountDownVisible = false);

	/** 展示窗口
	@param [out] dialogId 窗口ID值
	@param [in] timeOut 自动关闭时间，单位秒，如果不填则表示不会自动关闭
	@param [in] isCountDownVisible 是否显示倒计时
	*/
	void show(int32_t& dialogId, int32_t timeOut = -1, bool isCountDownVisible = false);

	/** 新增标签文字并设置
	@param [in] text 文本内容
	@param [in] rect 标签相对父窗口所在位置
	@param [in] textColor 文字颜色
	@return 返回标签指针
	*/
	Label* addLabel(const QString& text, const QRect& rect, const QColor& textColor);

	/** 新增提示文字并设置（多行）
	@param [in] text 文本内容
	@param [in] rect 标签相对父窗口所在位置
	@param [in] textColor 文字颜色
	@return 返回标签指针
	*/
	Label* addTip(const QString& text, const QRect& rect, const QColor& textColor);

	/** 新增按钮并设置，按钮点击后会关闭窗口
	@param [in] text 按钮内文本内容
	@param [in] rect 按钮相对父窗口所在位置
	@param [in] result 关闭窗口后的返回值
	@return 返回按钮指针
	*/
	COriginalButton* addButton(const QString& text, const QRect& rect, int32_t result);

	/** 添加分割线
	@param [in] point 起始点
	@param [in] length 长度
	@param [in] isHorizon 是否水平
	@param [in] upColor 上层颜色
	@param [in] downColor 下层颜色
	@return 返回分割线指针
	*/
	Label* addSeparator(const QPoint& point, int32_t length, bool isHorizon, const QColor upColor, const QColor downColor);

	/** 添加单行文本框
	@param [in] rect 标签相对父窗口所在位置
	@param [in] defaultText 默认显示文字
	@return 返回文本框指针
	*/
	LineEdit* addLineEdit(const QRect& rect, bool isPassword = false, const QString& defaultText = "");

	/** 添加密码框
	@param [in] rect 标签相对父窗口所在位置
	@param [in] defaultText 默认显示文字
	@return 返回文本框指针
	*/
	CPasswordInputBox* addPasswordInputBox(const QRect& rect, const QString& defaultText = "");

	/** 设置点击外部时会响应高亮闪烁的父窗口
	@param [in] parent 父窗口指针
	*/
	void setParentWindow(QWindow* parent);

Q_SIGNALS:
	void timeRest(int32_t seconds);

protected:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);
	void done(int result);

private slots:
	void endDialog();

protected:
	bool m_timeVisible;

private:
	int32_t m_timeRest;
	int32_t m_timeId;
	std::map<void*, int32_t> m_mapResult;
};