#pragma once
#include <map>
#include "DialogBase.h"
#include <QPropertyAnimation>
#include "QtControlsMacro.h"
#include "DialogResult.h"

class Label;
class PushButton;
class Separator;
/** 窗口基本模型，实现通用部分
*/
class QtControlsAPI DialogShow : public DialogBase
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

	/** 使用窗口内部结果存储，适用于show之后异步关闭的非模态窗口
	@param [in] result 初始窗口结果
	@param [in] userResult 初始用户自定义结果
	*/
	void useInternalResultStorage(DialogResult result = ERROR_RESULT, qint32 userResult = -1);

	/** 清空结果存储指针，避免窗口关闭后继续写入外部已经失效的内存
	*/
	void clearResultStorage();

	/** 判断关闭信号是否已经发出
	@return 返回true表示本轮显示周期已经发出过关闭信号
	*/
	bool closeSignalEmitted() const;

    /** 设置当按下空格和回车后窗口默认点击的按钮
    @param [in] button 默认点击的按钮指针
    */
    void initAcceptButton(PushButton* button);

    /** 设置返回值存储区
    @param [in] result 外部存储区指针
    */
    void setWindowResultAddr(DialogResult* result);

    /** 设置返回值
    @param [in] result 返回值，存入外部存储区
    */
    void setWindowResult(DialogResult result);

	/** 设置关闭按钮是否显示
	@param [in] visible 是否显示
	*/
	void setExitVisible(bool visible);

Q_SIGNALS:
    /** 在窗口关闭之后发送信号
    @param [in] result 窗口关闭时所给的返回值
    */
    void closedSignal(DialogResult* result);

protected slots:
    /** 处理DialogBase转发的键盘确认事件，触发默认按钮或关闭逻辑
    @param [in] tar 当前焦点所在控件指针
    @param [in] key 按下键的值
    */
    void onKeyboardAccept(QObject* tar, Qt::Key key);

protected:
    /** 显示时初始化按钮结果映射和倒计时展示
    @param [in] eve Qt显示事件
    */
    void showEvent(QShowEvent* eve);

    /** 关闭时写入窗口结果并发出closedSignal信号
    @param [in] eve Qt关闭事件
    */
    void closeEvent(QCloseEvent* eve);

    /** 检测关闭按钮、倒计时标签和结果存储区是否可用
    @return 返回true表示内部状态可用，false表示存在空指针或异常状态
    */
    bool check();

	/** 重置关闭信号状态，窗口重新显示时允许再次发送关闭信号
	*/
	void resetCloseState();
    //void escEvent();
    //void altF4PressedEvent();

private slots:
	/** 更新倒计时显示文字
	@param [in] timeOut 当前剩余秒数
	*/
	void onTimeUpdate(qint32 timeOut);

protected:
	// 右上角关闭按钮
	PushButton* m_exit;
	// 用户自定义返回值存储指针
	qint32* m_userResult;
	// 倒计时显示标签
    Label* m_time;
	// 弹窗结果存储指针
	DialogResult* m_result;
	// 非模态窗口内部保存的用户自定义返回值
	qint32 m_internalUserResult;
	// 非模态窗口内部保存的窗口返回值
	DialogResult m_internalResult;
	// 控件到弹窗结果的映射
    std::map<QWidget*, DialogResult> m_mapResult;
	// 回车或空格默认执行的按钮，由子类传入
    PushButton* m_acceptButton;
	// 当前结果指针是否指向窗口内部存储
	bool m_isUsingInternalResultStorage;
	// 当前显示周期是否已经发出过关闭信号
	bool m_closedSignalEmitted;
};