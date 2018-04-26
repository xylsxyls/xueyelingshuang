#pragma once
#include "DialogBase.h"
#include <QPropertyAnimation>
#include "ControlsMacro.h"

class Label;
class COriginalButton;
class Separator;
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
    @param [in] userParam 用户自定义参数
    */
    void setUserParam(int32_t userParam);

    /** 获取用户自定义参数
    @return 返回用户自定义参数
    */
    int32_t userParam();

    /** 设置当按下空格和回车后窗口默认点击的按钮
    @param [in] button 默认点击的按钮指针
    */
    void initAcceptButton(COriginalButton* button);

Q_SIGNALS:
    /** 在窗口关闭之后发送信号
    @param [in] result 窗口关闭时所给的返回值
    */
    void closedSignal(int32_t result);

public slots:
    void onKeyboardAccept(QObject* tar, Qt::Key key);

protected:
    void showEvent(QShowEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();

private slots:
	void onTimeUpdate(int32_t timeOut);
	
protected:
	COriginalButton* m_exit;
	int32_t m_userParam;
    Label* m_time;
    std::map<QWidget*, int32_t> m_mapResult;
    int32_t m_result;
    COriginalButton* m_acceptButton;
};