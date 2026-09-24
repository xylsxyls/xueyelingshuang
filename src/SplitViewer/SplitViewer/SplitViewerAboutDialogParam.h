#pragma once

#include "DialogManager/DialogManagerAPI.h"
#include "Config.h"
#include <QtCore/QRect>

/** SplitViewer关于窗口的内容参数。*/
struct SplitViewerAboutDialogParam : public CustomDialogParam
{
    /** 使用SplitViewer专用关于窗口类型初始化参数。*/
    SplitViewerAboutDialogParam();
    // 关于窗口正文
    QString message;
    // 关于窗口在桌面上需要居中的目标区域
    QRect centerRect;
};