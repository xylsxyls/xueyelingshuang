#include "SplitViewerConfigTests.h"
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include "QtControls/DialogBase.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "../../SplitViewer/SplitViewer/SplitViewerImageHelper.h"
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtTest/QtTest>
#include <cmath>
#include <limits>

bool SplitViewerConfigTests::runCase(int id, const QString& directory)
{
    bool passed = false;
    switch (id)
    {
    case 191: passed = shadowSize(directory); break;
    case 192: passed = invalidProfile(); break;
    case 193: passed = modelGuards(); break;
    case 194: passed = numericParsing(); break;
    case 195: passed = unicodeConversion(); break;
    case 196: passed = geometryBounds(); break;
    case 197: passed = imageCacheLifetime(); break;
    case 198: passed = boundedFileRead(directory); break;
    case 199: passed = packageBounds(); break;
    case 201: passed = serializationBounds(); break;
    default: return false;
    }
    QFile file(QDir(directory).filePath(QStringLiteral("config-review-") + QString::number(id) + QStringLiteral(".txt")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }
    QTextStream(&file) << "case=" << id << " passed=" << passed << "\n";
    return passed;
}

bool SplitViewerConfigTests::shadowSize(const QString& directory)
{
    DialogBase dialog;
    dialog.setGeometry(350, 280, 520, 340);
    dialog.setWindowShadow(true, 2);
    dialog.show();
    QTest::qWait(80);
    const QRect body = dialog.geometry();
    QWidget* shadow = dialog.findChild<QWidget*>(QStringLiteral("qtControlsDialogShadow"));
    if (shadow == nullptr)
    {
        return false;
    }
    const QRect shadowRect2 = shadow->geometry();
    dialog.setWindowShadow(true, 4);
    QTest::qWait(80);
    const QRect large = shadow->geometry();
    const QImage image = shadow->grab().toImage();
    const bool saved = image.save(QDir(directory).filePath(QStringLiteral("shadow-size4.png")));
    const bool expanded = shadow->isVisible() && large.width() > shadowRect2.width() &&
        large.height() > shadowRect2.height() && large.center() == shadowRect2.center() &&
        dialog.geometry() == body && dialog.windowShadowSize() == 4 &&
        qAlpha(image.pixel(image.width() / 2, image.height() / 2)) == 0;
    dialog.setWindowShadow(true, 2);
    QTest::qWait(40);
    return saved && expanded && shadow->geometry() == shadowRect2 && dialog.geometry() == body &&
        dialog.findChildren<QWidget*>(QStringLiteral("qtControlsDialogShadow")).size() == 1;
}

bool SplitViewerConfigTests::invalidProfile()
{
    const char* counts[] = { "-1", "257", "0", "0", "0", "0", "0", "1x", "2147483648" };
    const char* scales[] = { "1", "1", "0", "-2", "NaN", "1x", "1e500", "1", "1" };
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i)
    {
        // Handwritten UTF16 fixture is independent of the product serializer.
        const QString text = QStringLiteral("[SplitViewer]\r\nVersion=2\r\nLayerCount=") +
            QString::fromLatin1(counts[i]) + QStringLiteral("\r\n[Base]\r\nRoot=0\r\n[BaseNode0]\r\nKind=Leaf\r\nScale=") +
            QString::fromLatin1(scales[i]) + QStringLiteral("\r\n");
        std::vector<uint8_t> bytes;
        bytes.push_back(0xff);
        bytes.push_back(0xfe);
        for (int32_t j = 0; j < text.size(); ++j)
        {
            const ushort value = text.at(j).unicode();
            bytes.push_back(static_cast<uint8_t>(value & 0xff));
            bytes.push_back(static_cast<uint8_t>(value >> 8));
        }
        SplitViewerCoreDocument document;
        SplitViewerCoreNode* original = document.baseRoot();
        original->view.path = L"preserve.png";
        original->view.hasImage = true;
        document.addLayer();
        if (SplitViewerCoreDeserializeProfile(bytes, document) || document.baseRoot() != original ||
            document.layerCount() != 1 || document.selectedLayer() != 0 || original->view.path != L"preserve.png")
        {
            return false;
        }
    }
    return true;
}

bool SplitViewerConfigTests::modelGuards()
{
    SplitViewerCoreDocument document;
    document.setStageAspect((std::numeric_limits<double>::infinity)());
    if (!std::isfinite(document.stageAspect()) || document.stageAspect() <= 0)
    {
        return false;
    }
    document.setStageAspect((std::numeric_limits<double>::quiet_NaN)());
    if (!std::isfinite(document.stageAspect()) || document.stageAspect() <= 0)
    {
        return false;
    }
    SplitViewerCoreNode* root = document.baseRoot();
    root->makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    root->first->makeSplit(SPLITVIEWER_CORE_SPLIT_VERTICAL);
    SplitViewerCoreNode* subtree = root->first;
    return !document.deleteLeaf(root, subtree) && !root->isLeaf() && root->first == subtree &&
        !subtree->isLeaf() && subtree->first != nullptr && subtree->second != nullptr;
}

bool SplitViewerConfigTests::numericParsing()
{
    return CStringManager::parseInt32(L"2147483648", -7) == -7 &&
        CStringManager::parseInt32(L"-2147483649", -7) == -7 &&
        CStringManager::parseInt32(L"12x", -7) == -7 &&
        CStringManager::parseInt32(L" 42 \t", -7) == 42 &&
        CStringManager::parseFiniteDouble(L"NaN", -7.0) == -7.0 &&
        CStringManager::parseFiniteDouble(L"Inf", -7.0) == -7.0 &&
        CStringManager::parseFiniteDouble(L"1e500", -7.0) == -7.0 &&
        CStringManager::parseFiniteDouble(L"1.5px", -7.0) == -7.0 &&
        CStringManager::parseFiniteDouble(L" 1.25 \t", -7.0) == 1.25 &&
        CStringManager::formatFixedDouble(1.25, 2) == L"1.25" && invalidProfile();
}

bool SplitViewerConfigTests::unicodeConversion()
{
    // U+1F600的独立UTF-16LE代理对，前后为ASCII。
    const uint8_t fixture[] = { 0xff, 0xfe, 0x41, 0, 0x3d, 0xd8, 0, 0xde, 0x5a, 0 };
    const std::vector<uint8_t> bytes(fixture, fixture + sizeof(fixture));
    std::wstring wide;
    std::vector<uint8_t> encoded;
    if (!CStringManager::utf16LeToWide(bytes, wide) ||
        wide.size() != (sizeof(wchar_t) == 2 ? 4U : 3U) || wide.front() != L'A' || wide.back() != L'Z' ||
        !CStringManager::wideToUtf16Le(wide, encoded) || encoded != bytes)
    {
        return false;
    }
    const uint8_t malformed[][4] = { { 0xff, 0xfe, 0, 0xd8 }, { 0xff, 0xfe, 0, 0xdc }, { 0xfe, 0xff, 0, 0x41 } };
    for (size_t i = 0; i < sizeof(malformed) / sizeof(malformed[0]); ++i)
    {
        wide = L"preserve";
        const std::vector<uint8_t> bad(malformed[i], malformed[i] + 4);
        if (CStringManager::utf16LeToWide(bad, wide) || wide != L"preserve")
        {
            return false;
        }
    }
    std::vector<uint8_t> odd(bytes);
    odd.pop_back();
    if (CStringManager::utf16LeToWide(odd, wide) || wide != L"preserve" ||
        CStringManager::wideToUtf16Le(std::wstring(1, static_cast<wchar_t>(0xdc00)), encoded) || encoded != bytes)
    {
        return false;
    }
    SplitViewerCoreDocument document;
    document.baseRoot()->view.path = L"preserve.png";
    std::vector<uint8_t> profile;
    if (!SplitViewerCoreSerializeProfile(document, profile))
    {
        return false;
    }
    profile.push_back(0);
    profile.push_back(0xd8);
    return !SplitViewerCoreDeserializeProfile(profile, document) && document.baseRoot()->view.path == L"preserve.png";
}

bool SplitViewerConfigTests::geometryBounds()
{
    const double largest = (std::numeric_limits<double>::max)();
    const double smallest = (std::numeric_limits<double>::min)();
    if (SplitViewerCoreFitScale((std::numeric_limits<double>::quiet_NaN)(), 100, 100, 100) != 1.0 ||
        SplitViewerCoreFitScale(100, 100, (std::numeric_limits<double>::infinity)(), 100) != 1.0 ||
        SplitViewerCoreFitScale(largest, largest, smallest, smallest) != 1.0)
    {
        return false;
    }
    SplitViewerCoreLeafState view;
    view.hasImage = true;
    view.autoFit = false;
    view.scale = 2.0;
    view.offsetX = 10.0;
    view.offsetY = -20.0;
    SplitViewerCoreResizeView(view, smallest, smallest, largest, largest);
    if (view.scale != 2.0 || view.offsetX != 10.0 || view.offsetY != -20.0)
    {
        return false;
    }
    SplitViewerCoreZoom(view, largest, 120, false);
    if (!std::isfinite(view.scale) || view.scale <= 0)
    {
        return false;
    }
    SplitViewerCoreNode node;
    node.makeSplit(static_cast<SplitViewerCoreSplitDirection>(-1));
    return node.isLeaf() && node.first == nullptr && node.second == nullptr;
}

bool SplitViewerConfigTests::imageCacheLifetime()
{
    SplitViewerCoreDocument document;
    document.baseRoot()->view.path = L"shared.png";
    document.baseRoot()->view.hasImage = true;
    SplitViewerCoreLayer* layer = document.addLayer();
    layer->root->view.path = L"shared.png";
    layer->root->view.hasImage = true;
    QMap<QString, QImage> images;
    images.insert(QStringLiteral("shared.png"), QImage(2, 2, QImage::Format_RGB32));
    images.insert(QStringLiteral("unused.png"), QImage(2, 2, QImage::Format_RGB32));
    SplitViewerImageHelper::pruneUnusedImages(document, images);
    if (images.size() != 1 || !images.contains(QStringLiteral("shared.png")))
    {
        return false;
    }
    document.baseRoot()->view.clear();
    SplitViewerImageHelper::pruneUnusedImages(document, images);
    if (images.size() != 1)
    {
        return false;
    }
    document.deleteLayer(0);
    SplitViewerImageHelper::pruneUnusedImages(document, images);
    return images.isEmpty();
}

bool SplitViewerConfigTests::boundedFileRead(const QString& directory)
{
    const QString path = QDir(directory).filePath(QStringLiteral("限长 读取.bin"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("abcd", 4) != 4 || !file.flush())
    {
        return false;
    }
    file.close();
    std::vector<uint8_t> bytes(1, 99);
    const uint8_t expectedData[] = { 'a', 'b', 'c', 'd' };
    const std::vector<uint8_t> expected(expectedData, expectedData + 4);
    if (!CSystem::readBinaryFile(path.toStdWString(), 4, bytes) || bytes != expected ||
        CSystem::readBinaryFile(path.toStdWString(), 3, bytes) || bytes != expected ||
        CSystem::readBinaryFile((path + QStringLiteral(".missing")).toStdWString(), 4, bytes) || bytes != expected)
    {
        return false;
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    file.close();
    return CSystem::readBinaryFile(path.toStdWString(), 0, bytes) && bytes.empty();
}

bool SplitViewerConfigTests::packageBounds()
{
    QImage image(2, 2, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QByteArray pngBytes;
    QBuffer buffer(&pngBytes);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG"))
    {
        return false;
    }
    const uint8_t* start = reinterpret_cast<const uint8_t*>(pngBytes.constData());
    const std::vector<uint8_t> png(start, start + pngBytes.size());
    const uint8_t raw[] = { 0xff, 0xfe, 'A', 0 };
    const std::vector<uint8_t> config(raw, raw + sizeof(raw));
    std::vector<uint8_t> inplace(png);
    if (!SplitViewerCoreBuildConfigPackage(inplace, config, inplace) ||
        QImage::fromData(&inplace[0], static_cast<int>(inplace.size()), "PNG").isNull() ||
        !SplitViewerCoreExtractEmbeddedConfig(inplace, inplace) || inplace != config)
    {
        return false;
    }
    const std::vector<uint8_t> sentinel(3, 77);
    std::vector<uint8_t> output(sentinel);
    std::vector<uint8_t> tooLarge(16 * 1024 * 1024 + 1, 0);
    std::vector<uint8_t> malformed(png);
    if (malformed.size() < 12)
    {
        return false;
    }
    const size_t end = malformed.size() - 12;
    malformed[end + 3] = 1;
    malformed.insert(malformed.begin() + end + 8, 0);
    return !SplitViewerCoreBuildConfigPackage(sentinel, config, output) && output == sentinel &&
        !SplitViewerCoreBuildConfigPackage(png, tooLarge, output) && output == sentinel &&
        !SplitViewerCoreBuildConfigPackage(malformed, config, output) && output == sentinel &&
        !SplitViewerCoreExtractEmbeddedConfig(png, output) && output == sentinel;
}

bool SplitViewerConfigTests::serializationBounds()
{
    const std::vector<uint8_t> sentinel(3, 55);
    std::vector<uint8_t> bytes(sentinel);
    SplitViewerCoreDocument deep;
    SplitViewerCoreNode* leaf = deep.baseRoot();
    for (int i = 0; i < 129; ++i)
    {
        leaf->makeSplit(SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
        leaf = leaf->first;
    }
    if (SplitViewerCoreSerializeProfile(deep, bytes) || bytes != sentinel)
    {
        return false;
    }
    SplitViewerCoreDocument layers;
    for (int i = 0; i < 257; ++i)
    {
        layers.addLayer();
    }
    if (SplitViewerCoreSerializeProfile(layers, bytes) || bytes != sentinel)
    {
        return false;
    }
    SplitViewerCoreDocument valid;
    return SplitViewerCoreSerializeProfile(valid, bytes) && bytes.size() > 2 && bytes[0] == 0xff && bytes[1] == 0xfe;
}