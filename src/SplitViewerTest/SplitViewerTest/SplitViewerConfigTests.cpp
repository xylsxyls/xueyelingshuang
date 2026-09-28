#include "SplitViewerConfigTests.h"
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include "QtControls/DialogBase.h"
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtTest/QtTest>
#include <cmath>
#include <limits>

bool SplitViewerConfigTests::runCase(int id, const QString& directory)
{
    const bool passed = id == 191 ? shadowSize(directory) :
        id == 192 ? invalidProfile() : id == 193 && modelGuards();
    QFile file(QDir(directory).filePath(QStringLiteral("config-review.txt")));
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
    const char* counts[] = { "-1", "257", "0", "0" };
    const char* scales[] = { "1", "1", "0", "-2" };
    for (int32_t i = 0; i < 4; ++i)
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