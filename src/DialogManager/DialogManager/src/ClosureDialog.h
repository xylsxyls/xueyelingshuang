#pragma once
#include "BoxDialogBase.h"
#include "DialogManagerMacro.h"

class LockedAccountPanel;
/** 封号信息弹框，承载被封禁账号列表面板
*/
class DialogManagerAPI ClosureDialog : public BoxDialogBase
{
public:
	/** 构造函数
	*/
	ClosureDialog();

    /** 获取内部界面指针
    @return 返回内部界面指针
    */
    LockedAccountPanel* lockedAccountPanel();

protected:
    /** 调整封号列表面板布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查内部控件是否已经创建完成
    @return 返回true表示控件有效
    */
    bool check();

private:
	// 被封禁账号列表面板
	LockedAccountPanel* m_lockedAccountPanel;
};