#pragma once

#include "DialogManager/DialogManagerAPI.h"

class QResizeEvent;

/** 关于窗口外壳；由DialogManager持有，负责标题和关闭交互 */
class SplitViewerAboutDialog : public CustomDialog
{
public:
    /** 创建标题栏和细线关闭图标 */
    SplitViewerAboutDialog();

    /** 初始化主体并按画布区域居中
    @param [in] param 关于窗口参数，初始化期间借用
    @return 基类初始化成功时返回true
    */
    bool initDialog(const DialogParam& param) override;

protected:
    /** 标题栏尺寸变化时同步右上角关闭按钮的位置
    @param [in] event Qt尺寸事件
    */
    void resizeEvent(QResizeEvent* event) override;

private:
    /** 按Config摆放标题栏右上角的叉号按钮 */
    void updateTitleCloseButtonGeometry();
};