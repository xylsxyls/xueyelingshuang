#pragma once

#include "DialogManager/DialogManagerAPI.h"

class QResizeEvent;

class SplitViewerAboutDialog : public CustomDialog
{
public:
    SplitViewerAboutDialog();
    bool initDialog(const DialogParam& param) override;

protected:
    /** 标题栏尺寸变化时同步右上角关闭按钮的位置。 */
    void resizeEvent(QResizeEvent* event) override;

private:
    /** 配置并摆放标题栏右上角的叉号按钮。 */
    void updateTitleCloseButtonGeometry();
};