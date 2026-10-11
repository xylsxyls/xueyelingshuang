#pragma once

#include "Config.h"
#include "DialogManager/DialogManagerAPI.h"
#include <QtCore/QRect>

/** SplitViewer关于窗口的内容参数
*/
struct SplitViewerAboutDialogParam : public CustomDialogParam
{
public:
    // 关于窗口正文
    QString m_message;
    // 关于窗口在桌面上需要居中的目标区域
    QRect m_centerRect;

public:
    /** 使用SplitViewer专用关于窗口类型初始化参数
    */
    SplitViewerAboutDialogParam();
};