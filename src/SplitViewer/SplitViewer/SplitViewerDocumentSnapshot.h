#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtGui/QImage>
#include <map>

/** 后台请求的独立场景和资源，完成后只在GUI一次性交付
*/
struct SplitViewerDocumentSnapshot
{
public:
    // 独占文档，不借用当前窗口的树
    SplitViewerCoreDocument m_document;
    // 原节点身份与结果节点关联，原指针只比较不解引用
    std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*> m_nodes;
    // 隐式共享图像值；写时分离，不借用GUI缓存
    QMap<QString, QImage> m_images;
    // 需要释放覆盖目标资源的原节点，完成前不释放现场
    const SplitViewerCoreNode* m_releaseTarget;
    // 失败时向GUI呈现的诊断
    QString m_error;

public:
    /** 以已经验证的配置初始化独立场景
    @param [in] config 应用在GUI初始化阶段复制的Core配置
    */
    explicit SplitViewerDocumentSnapshot(const SplitViewerCoreConfig& config);
};