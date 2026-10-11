#include "SplitViewerMoveVideoTests.h"
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QTextStream>

bool SplitViewerMoveVideoTests::directionalTargets()
{
    SplitViewerCoreDocument document;
    SplitViewerCoreNode* root = document.baseRoot();
    root->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    SplitViewerCoreNode* left = root->first;
    root->second->makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    SplitViewerCoreNode* topRight = root->second->first;
    SplitViewerCoreNode* bottomRight = root->second->second;
    if (SplitViewerCoreDirectionalTarget(root, left, SPLITVIEWER_CORE_MOVE_RIGHT) != topRight ||
        SplitViewerCoreDirectionalTarget(root, bottomRight, SPLITVIEWER_CORE_MOVE_LEFT) != left ||
        SplitViewerCoreDirectionalTarget(root, topRight, SPLITVIEWER_CORE_MOVE_DOWN) != bottomRight ||
        SplitViewerCoreDirectionalTarget(root, bottomRight, SPLITVIEWER_CORE_MOVE_UP) != topRight ||
        SplitViewerCoreDirectionalTarget(root, left, SPLITVIEWER_CORE_MOVE_LEFT) != nullptr)
    {
        return false;
    }
    SplitViewerCoreDocument horizontal;
    root = horizontal.baseRoot();
    root->makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    root->second->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    return SplitViewerCoreDirectionalTarget(root, root->first, SPLITVIEWER_CORE_MOVE_DOWN) == root->second->first &&
        SplitViewerCoreDirectionalTarget(root, root->second->second, SPLITVIEWER_CORE_MOVE_UP) == root->first;
}

bool SplitViewerMoveVideoTests::invalidMoves()
{
    SplitViewerCoreDocument document;
    SplitViewerCoreNode* root = document.baseRoot();
    root->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    root->first->view.path = L"image.png";
    root->first->view.hasImage = true;
    root->first->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    SplitViewerCoreLayer* layer = document.addLayer();
    const bool rejected = !SplitViewerCoreMoveContent(root, root->first, layer->root) &&
        !SplitViewerCoreMoveContent(root, root->first, root->first) &&
        !SplitViewerCoreMoveContent(root, root->second, root->first) &&
        !SplitViewerCoreMoveContent(root, root->first, root) &&
        SplitViewerCoreDirectionalTarget(root, layer->root, SPLITVIEWER_CORE_MOVE_RIGHT) == nullptr &&
        SplitViewerCoreDirectionalTarget(root, root->first, static_cast<SplitViewerCoreMoveDirection>(99)) == nullptr;
    return rejected && root->first->view.path == L"image.png" && root->first->view.hasImage &&
        !root->second->view.hasContent() && !layer->root->view.hasContent();
}

bool SplitViewerMoveVideoTests::contentMove()
{
    SplitViewerCoreDocument document;
    SplitViewerCoreNode* root = document.baseRoot();
    root->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    SplitViewerCoreNode* source = root->first;
    SplitViewerCoreNode* target = root->second;
    source->view.path = L"原图片.png";
    source->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    source->view.hasImage = true;
    source->view.autoFit = false;
    source->view.scale = 2.0;
    source->view.offsetX = 17.0;
    source->view.offsetY = -9.0;
    target->view.path = L"target.png";
    target->view.hasImage = true;
    target->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    if (!SplitViewerCoreMoveContent(root, source, target) || source->view.hasContent() || !source->view.path.empty() ||
        !source->view.autoFit || source->view.scale != 1.0 || target->view.path != L"原图片.png" ||
        !target->view.hasImage || target->view.autoFit || target->view.scale != 2.0 || target->view.offsetX != 17.0 ||
        target->view.offsetY != -9.0)
    {
        return false;
    }
    target->view.clear();
    target->view.path = L"视频 sample.avi";
    target->view.contentKind = SPLITVIEWER_CORE_CONTENT_VIDEO;
    return SplitViewerCoreMoveContent(root, target, source) && root->first == source && root->second == target &&
        !root->isLeaf() && source->view.contentKind == SPLITVIEWER_CORE_CONTENT_VIDEO &&
        source->view.path == L"视频 sample.avi" && !source->view.hasImage && !target->view.hasContent();
}

bool SplitViewerMoveVideoTests::videoProfile()
{
    SplitViewerCoreDocument document;
    document.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    SplitViewerCoreNode* video = document.baseRoot()->second;
    video->view.contentKind = SPLITVIEWER_CORE_CONTENT_VIDEO;
    video->view.path = L"D:/测试 空格/sample.avi";
    document.baseRoot()->first->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    document.baseRoot()->first->view.hasImage = true;
    document.baseRoot()->first->view.path = L"picture.jfif";
    std::vector<uint8_t> bytes;
    if (!SplitViewerCoreSerializeProfile(document, bytes) || bytes.size() < 2)
    {
        return false;
    }
    QString text;
    for (size_t i = 2; i + 1 < bytes.size(); i += 2)
    {
        text.append(QChar(static_cast<ushort>(bytes[i] | (static_cast<unsigned>(bytes[i + 1]) << 8))));
    }
    const int section = text.indexOf(QStringLiteral("[BaseNode2]"));
    if (section < 0 || !text.mid(section).contains(QStringLiteral("ContentKind=3")) ||
        !text.mid(section).contains(QStringLiteral("Path=D:/测试 空格/sample.avi")))
    {
        return false;
    }
    SplitViewerCoreDocument restored;
    const bool roundTrip = SplitViewerCoreDeserializeProfile(bytes, restored) &&
        restored.baseRoot()->second->view.contentKind == SPLITVIEWER_CORE_CONTENT_VIDEO &&
        restored.baseRoot()->second->view.path == video->view.path && !restored.baseRoot()->second->view.hasImage &&
        restored.baseRoot()->first->view.path == L"picture.jfif" && restored.baseRoot()->first->view.hasImage;
    if (!roundTrip)
    {
        return false;
    }
    video->view.path.clear();
    std::vector<uint8_t> invalidOutput(1, 17);
    if (SplitViewerCoreSerializeProfile(document, invalidOutput) || invalidOutput.size() != 1 || invalidOutput[0] != 17)
    {
        return false;
    }
    video->view.contentKind = static_cast<SplitViewerCoreContentKind>(99);
    return !SplitViewerCoreSerializeProfile(document, invalidOutput) && invalidOutput.size() == 1 &&
        invalidOutput[0] == 17;
}

bool SplitViewerMoveVideoTests::run(int id, const QString& directory, QTextStream& log)
{
    Q_UNUSED(directory);
    log << "move/video case=" << id << "\n";
    switch (id)
    {
    case 203:
        return directionalTargets();
    case 204:
        return invalidMoves();
    case 205:
        return contentMove();
    case 206:
        return videoProfile();
    default:
        return false;
    }
}