#pragma once
#include "DialogManagerMacro.h"
#include "PopDialog.h"

/** 等待弹出框
*/
class WaitDialog : public PopDialog
{
	Q_OBJECT
public:
    /** 构造函数
    */
    WaitDialog();

    /** 设置提示内容
    @param [in] tip 提示内容
    */
    void setTip(const QString& tip);

protected:
    void resizeEvent(QResizeEvent* eve);
    bool check();

private:
	Label* m_tip;
};