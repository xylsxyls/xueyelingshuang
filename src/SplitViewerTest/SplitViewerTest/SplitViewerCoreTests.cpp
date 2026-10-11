#include "SplitViewerCoreTests.h"
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QBuffer>
#include <QtGui/QImage>
#include <QtGui/QColor>
#include <vector>

bool SplitViewerCoreTests::run(int32_t id)
{
    std::vector<bool> results;
    SplitViewerCoreDocument document;
    document.baseRoot()->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    const bool splitOk = document.baseRoot()->kind == SPLITVIEWER_CORE_NODE_SPLIT && document.baseRoot()->first &&
        document.baseRoot()->second;
    results.push_back(splitOk);

    SplitViewerCoreLayer* layer = document.addLayer();
    const bool layerOk = layer && document.layerCount() == 1 && document.selectedLayer() == 0;
    results.push_back(layerOk);

    SplitViewerCoreDocument layerDeleteDocument;
    SplitViewerCoreLayer* deletableLayer = layerDeleteDocument.addLayer();
    if (deletableLayer)
    {
        deletableLayer->root->makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    }
    const bool deleteLayerOk = deletableLayer != nullptr && layerDeleteDocument.deleteLayer(0) &&
        layerDeleteDocument.layerCount() == 0 && layerDeleteDocument.selectedLayer() == -1;
    results.push_back(deleteLayerOk);

    SplitViewerCoreRect first;
    SplitViewerCoreRect splitter;
    SplitViewerCoreRect second;
    SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(), document.baseRoot(), first, splitter, second);
    const bool geometryOk = first.width() > 0.0 && second.width() > 0.0 && splitter.width() > 0.0;
    results.push_back(geometryOk);

    document.baseRoot()->first->view.path = L"test-image.png";
    document.baseRoot()->first->view.hasImage = true;
    document.baseRoot()->first->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    std::vector<uint8_t> profile;
    const bool serializeOk = SplitViewerCoreSerializeProfile(document, profile);
    SplitViewerCoreDocument restored;
    const bool deserializeOk = serializeOk && SplitViewerCoreDeserializeProfile(profile, restored) &&
        restored.layerCount() == 1 && restored.baseRoot()->kind == SPLITVIEWER_CORE_NODE_SPLIT;
    results.push_back(deserializeOk);

    QImage thumbnail(16, 16, QImage::Format_ARGB32_Premultiplied);
    thumbnail.fill(QColor(32, 128, 192));
    QByteArray pngBytes;
    QBuffer pngBuffer(&pngBytes);
    pngBuffer.open(QIODevice::WriteOnly);
    thumbnail.save(&pngBuffer, "PNG");
    std::vector<uint8_t> png(reinterpret_cast<const uint8_t*>(pngBytes.constData()),
        reinterpret_cast<const uint8_t*>(pngBytes.constData()) + pngBytes.size());
    std::vector<uint8_t> package;
    std::vector<uint8_t> extracted;
    const bool packageOk = SplitViewerCoreBuildConfigPackage(png, profile, package) &&
        SplitViewerCoreExtractEmbeddedConfig(package, extracted) && extracted == profile;
    results.push_back(packageOk);

    const bool scaleOk = SplitViewerCoreFitScale(400.0, 200.0, 200.0, 200.0) == 0.5;
    results.push_back(scaleOk);
    return id >= 1 && static_cast<size_t>(id) <= results.size() && results[static_cast<size_t>(id - 1)];
}