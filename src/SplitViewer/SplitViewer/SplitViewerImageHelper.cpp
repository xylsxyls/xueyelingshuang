#include "SplitViewerImageHelper.h"
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtGui/QImage>

QString SplitViewerImageHelper::path(const std::wstring& value)
{
    return QString::fromStdWString(value);
}

void SplitViewerImageHelper::setImageStatus(SplitViewerCoreNode* node, QMap<QString, QImage>& cache)
{
    if (!node)
    {
        return;
    }
    if (node->isLeaf())
    {
        if (node->view.contentKind == SPLITVIEWER_CORE_CONTENT_EMBEDDED)
        {
            node->view.clear();
        }
        if (!node->view.path.empty())
        {
            const QString path = SplitViewerImageHelper::path(node->view.path);
            if (!cache.contains(path))
            {
                cache.insert(path, QImage(path));
            }
            node->view.hasImage = !cache.value(path).isNull();
            if (node->view.hasImage)
            {
                node->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
            }
            else
            {
                node->view.clear();
            }
        }
        return;
    }
    SplitViewerImageHelper::setImageStatus(node->first, cache);
    SplitViewerImageHelper::setImageStatus(node->second, cache);
}

void SplitViewerImageHelper::pruneUnusedImages(const SplitViewerCoreDocument& document, QMap<QString, QImage>& cache)
{
    std::vector<SplitViewerCoreNode*> leaves;
    SplitViewerCoreNode::collectLeaves(document.baseRoot(), leaves);
    for (int index = 0; index < document.layerCount(); ++index)
    {
        SplitViewerCoreNode::collectLeaves(document.layerAt(index)->root, leaves);
    }
    QSet<QString> used;
    for (size_t index = 0; index < leaves.size(); ++index)
    {
        if (leaves[index]->view.hasImage)
        {
            used.insert(path(leaves[index]->view.path));
        }
    }
    for (QMap<QString, QImage>::iterator it = cache.begin(); it != cache.end();)
    {
        if (!used.contains(it.key()))
        {
            it = cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}