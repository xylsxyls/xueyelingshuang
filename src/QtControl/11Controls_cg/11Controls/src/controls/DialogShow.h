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
    void setTimeRestVisible(bool visible);

    void setUserParam(int32_t userParam);

    int32_t userParam();

    /** 设置当按下空格和回车后窗口默认点击的按钮
    */
    void initKeyboardAccept(COriginalButton* button);

Q_SIGNALS:
	/** 当通知框窗口关闭时发送信号，在关闭动画执行之前发送信号
	@param [in] dialogId 窗口ID
    @param [in] userId 用户自定义ID
	@param [in] result 窗口关闭时所给的返回值
	@param [in] userParam 用户自定义值
	*/
	void dialogDone(int32_t dialogId, int32_t userId, int32_t result, int32_t userParam);

public:
    void endDialog();

protected:
    void showEvent(QShowEvent* eve);

private slots:
	void onTimeUpdate(int32_t timeOut);
    void onKeyboardAccept(QObject* tar, Qt::Key key);
	
protected:
	COriginalButton* m_exit;
	int32_t m_userParam;
    Label* m_time;
    std::map<QWidget*, int32_t> m_mapResult;
    int32_t m_result;

private:
    static std::wstring s_countDownString;
    COriginalButton* m_acceptButton;
};