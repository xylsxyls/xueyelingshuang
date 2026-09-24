#pragma once

#include "DialogManager/DialogManagerAPI.h"

class PushButton;

/** SplitViewer关于窗口内容区，负责局部视觉样式和关闭交互。*/
class SplitViewerAboutDialogView : public CustomViewBase
{
public:
    /** 创建空的关于窗口内容区。*/
    SplitViewerAboutDialogView();

    /** 按关于窗口参数创建控件和样式。
    @param [in] param 必须为SplitViewerAboutDialogParam
    @return 参数类型正确且创建成功时返回true
    */
    bool initView(const DialogParam& param) override;

    /** 返回关于窗口默认尺寸。
    @return 逻辑像素尺寸
    */
    QSize preferredSize() const override;

    /** 返回默认获得焦点的关闭按钮。
    @return 内容区持有的按钮，不转移所有权
    */
    QWidget* defaultFocusWidget() const override;

private:
    // 关闭按钮，由Qt父子关系持有
    PushButton* m_closeButton;
};