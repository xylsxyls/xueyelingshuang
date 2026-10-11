#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QString>

/** 启动请求尚未提交的目标；节点由主窗口文档借用
*/
struct SplitViewerVideoTarget
{
public:
    // 借用主窗口文档中的目标，提交前重新核验
    SplitViewerCoreNode* m_node;
    // 待加载视频的绝对路径
    QString m_path;

public:
    /** 初始化空目标，不持有Core节点
    */
    SplitViewerVideoTarget();

    /** 记录本次加载意图
    @param [in] target 文档持有的目标叶子
    @param [in] mediaPath 媒体绝对路径
    */
    SplitViewerVideoTarget(SplitViewerCoreNode* target, const QString& mediaPath);
};