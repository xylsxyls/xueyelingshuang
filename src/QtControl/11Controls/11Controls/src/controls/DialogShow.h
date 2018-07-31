#pragma once
#include "DialogBase.h"
#include <QPropertyAnimation>
#include "ControlsMacro.h"

class Label;
class COriginalButton;
class Separator;
enum DialogResult;
/** 窗口基本模型，实现通用部分
*/
class ControlsAPI DialogShow : public DialogBase
{
	Q_OBJECT
public:
	/** 构造函数
	*/
	DialogShow();

	/** 析构函数
	*/
	virtual ~DialogShow();

public:
    /** 设置剩余时间是否显示
    @param [in] visible 是否显示
    */
    void setTimeRestVisible(bool visible);

    /** 设置用户自定义参数
    @param [in] userResult 用户自定义参数
    */
    void setUserResult(qint32 userResult);

    /** 获取用户自定义参数
    @return 返回用户自定义参数
    */
    qint32 userResult();

	/** 设置用户自定义参数指针
	@param [in] userResult 用户自定义参数
	*/
	void setUserResultPtr(qint32* userResult);

	/** 获取存储用户自定义值的指针
	@return 返回存储用户自定义值的指针
	*/
	qint32* userResultPtr();

    /** 设置当按下空格和回车后窗口默认点击的按钮
    @param [in] button 默认点击的按钮指针
    */
    void initAcceptButton(COriginalButton* button);

    /** 设置返回值存储区
    @param [in] result 外部存储区指针
    */
    void setWindowResultAddr(DialogResult* result);

    /** 设置返回值
    @param [in] result 返回值，存入外部存储区
    */
    void setWindowResult(DialogResult result);

Q_SIGNALS:
    /** 在窗口关闭之后发送信号
    @param [in] result 窗口关闭时所给的返回值
    */
    void closedSignal(DialogResult* result);

protected slots:
    void onKeyboardAccept(QObject* tar, Qt::Key key);

protected:
    void showEvent(QShowEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();
    void escEvent();
    void altF4PressedEvent();

private slots:
	void onTimeUpdate(qint32 timeOut);
	
protected:
	COriginalButton* m_exit;
	qint32* m_userResult;
    Label* m_time;
	DialogResult* m_result;
    std::map<QWidget*, DialogResult> m_mapResult;
	//或者和空格默认执行的按钮，子类传送指针
    COriginalButton* m_acceptButton;
};