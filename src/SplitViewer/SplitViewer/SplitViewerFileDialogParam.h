#pragma once
#include "DialogManager/DialogManagerAPI.h"
#include <QtCore/QSharedPointer>

/** 同步文件选择参数，结果共享持有，不借用调用方栈变量 */
struct SplitViewerFileDialogParam : public CustomDialogParam
{
public:
    // 是否选择保存路径
    bool save;
    // 初始文件或目录
    QString initial;
    // Qt文件名过滤规则
    QString filter;
    // 只有确认时才填写的结果
    QSharedPointer<QString> selectedPath;

public:
    /** 初始化打开文件模式及独立结果 */
    SplitViewerFileDialogParam();
};