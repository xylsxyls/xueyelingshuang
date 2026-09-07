#pragma once
#include "DialogManagerMacro.h"
#include "PopDialog.h"

/** 等待弹出框，展示不可交互的等待提示
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
    /** 调整等待提示文本布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private:
	// 等待提示文本
	Label* m_tip;
};