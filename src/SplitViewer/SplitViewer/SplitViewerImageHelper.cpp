#include "SplitViewerImageHelper.h"
#include "SplitViewerCoreAPI.h"
#include <QtCore/QMap>
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
        if (node->view.contentKind == SPLITVIEWER_CORE_CONTENT_EMBEDDED) node->view.clear();
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