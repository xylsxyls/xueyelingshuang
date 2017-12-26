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
	@param [in] timeOut 自动关闭时间，单位秒，如果不填则表示不会自动关闭
	@return 关闭窗口时给的返回值
	*/
	int32_t exec(int32_t timeOut = -1);

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
	@param [in] width 宽度
	@param [in] isHorizon 是否水平
	@return 返回分割线指针
	*/
	Label* addSeparator(const QPoint& point, int32_t width, bool isHorizon);

	/** 添加单行文本框
	@param [in] rect 标签相对父窗口所在位置
	@param [in] defaultText 默认显示文字
	@return 返回文本框指针
	*/
	LineEdit* addLineEdit(const QRect& rect, const QString& defaultText = "");

	/** 添加密码框
	@param [in] rect 标签相对父窗口所在位置
	@param [in] defaultText 默认显示文字
	@return 返回文本框指针
	*/
	CPasswordInputBox* addPasswordInputBox(const QRect& rect, const QString& defaultText = "");

	/** 设置点击外部时会响应高亮闪烁的父窗口
	@param [in] parent 父窗口指针
	*/
	void setResponseHighlightDialog(QDialog* parent);

Q_SIGNALS:
	void timeRest(int32_t seconds);

private slots:
	void endDialog();

private:
	void showEvent(QShowEvent* eve);
	void timerEvent(QTimerEvent* eve);

private:
	int32_t m_timeRest;
	int32_t m_timeId;
	std::map<void*, int32_t> m_mapResult;
};