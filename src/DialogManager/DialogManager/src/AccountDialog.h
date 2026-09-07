#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class Label;
class ComboBox;
class LineEdit;
/** 添加账号弹框，负责收集子账号名称并向调用方返回确认或取消结果
*/
class DialogManagerAPI AccountDialog : public BoxDialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	AccountDialog();

	/** 设置账号错误提示是否显示
	@param [in] visible 是否显示
	*/
	void setErrorVisible(bool visible);

	/** 清空输入框
	*/
	void clearAccountEdit();

	/** 获取输入框内容
	@return 返回输入框内容
	*/
	QString accountEditText();

    /** 设置确定按钮按下的返回值
    @param [in] result 确定按钮按下的返回值
    */
    void setAcceptDown(DialogResult result);

    /** 设置取消按钮按下的返回值
    @param [in] result 取消按钮按下的返回值
    */
    void setIgnoreDown(DialogResult result);

Q_SIGNALS:
    /** 错误提示是否显示
    @param [in] visible 是否显示
    */
	void errorVisible(bool visible);

protected:
	/** 处理账号输入框焦点变化，切换占位文字和输入样式
	@param [in] tar 触发事件的对象
	@param [in] eve 事件对象
	@return 返回true表示事件已被过滤，否则返回false
	*/
	bool eventFilter(QObject* tar, QEvent* eve);

    /** 调整内部控件布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private slots:
	/** 处理回车确认逻辑，避免空格在账号框内触发确认
	@param [in] tar 当前焦点对象
	@param [in] key 按下的按键
	*/
	void accountAccept(QObject* tar, Qt::Key key);

private:
	// 标题区和内容区之间的分割线
	Label* m_sep;

	// 注册账号标题文本
	Label* m_registerAlt;

	// 账号名称输入框
	LineEdit* m_account;

	// 账号输入错误提示
	Label* m_errorAccount;

	// 确定按钮
	COriginalButton* m_accept;

	// 取消按钮
	COriginalButton* m_ignore;
};