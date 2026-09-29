#include "SplitViewerCoreProfileHelper.h"
#include "SplitViewerCore.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreGeometryHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <memory>

std::wstring SplitViewerCoreProfileHelper::formatDouble(double value)
{
    return CStringManager::formatFixedDouble(value, 10);
}

void SplitViewerCoreProfileHelper::setValue(SplitViewerCoreProfile& profile,
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& value)
{
    profile.values[section][key] = value;
}

std::wstring SplitViewerCoreProfileHelper::getValue(const SplitViewerCoreProfile& profile,
    const std::wstring& section,
    const std::wstring& key,
    const std::wstring& defaultValue)
{
    std::map<std::wstring, std::map<std::wstring, std::wstring> >::const_iterator sectionIt = profile.values.find(section);
    if (sectionIt == profile.values.end())
    {
        return defaultValue;
    }
    std::map<std::wstring, std::wstring>::const_iterator valueIt = sectionIt->second.find(key);
    return valueIt == sectionIt->second.end() ? defaultValue : valueIt->second;
}

bool SplitViewerCoreProfileHelper::saveNode(SplitViewerCoreProfile& profile,
    const std::wstring& prefix,
    const SplitViewerCoreNode* node,
    int& nextId, int depth)
{
    if (node == nullptr || depth > SplitViewerCoreConfig::kMaximumTreeDepth ||
        nextId >= SplitViewerCoreConfig::kMaximumNodes)
    {
        return false;
    }
    const int id = nextId++;
    const std::wstring section = prefix + L"Node" + CStringManager::Format(L"%d", id);
    if (node->isLeaf())
    {
        setValue(profile, section, L"Kind", L"Leaf");
        setValue(profile, section, L"Path", node->view.path);
        setValue(profile, section, L"HasImage", CStringManager::Format(L"%d", node->view.hasImage ? 1 : 0));
        setValue(profile, section, L"AutoFit", CStringManager::Format(L"%d", node->view.autoFit ? 1 : 0));
        setValue(profile, section, L"Scale", formatDouble(node->view.scale));
        setValue(profile, section, L"OffsetX", formatDouble(node->view.offsetX));
        setValue(profile, section, L"OffsetY", formatDouble(node->view.offsetY));
        setValue(profile, section, L"ContentKind", CStringManager::Format(L"%d", static_cast<int>(node->view.contentKind)));
        return true;
    }

    setValue(profile, section, L"Kind", L"Split");
    setValue(profile, section, L"Direction",
        CStringManager::Format(L"%d", node->direction == SPLITVIEWER_CORE_SPLIT_HORIZONTAL ? 0 : 1));
    setValue(profile, section, L"Ratio", formatDouble(node->ratio));
    const int firstId = nextId;
    if (!saveNode(profile, prefix, node->first, nextId, depth + 1))
    {
        return false;
    }
    const int secondId = nextId;
    if (!saveNode(profile, prefix, node->second, nextId, depth + 1))
    {
        return false;
    }
    setValue(profile, section, L"First", CStringManager::Format(L"%d", firstId));
    setValue(profile, section, L"Second", CStringManager::Format(L"%d", secondId));
    return true;
}

SplitViewerCoreNode* SplitViewerCoreProfileHelper::loadNode(const SplitViewerCoreProfile& profile,
    const std::wstring& prefix,
    int id,
    int depth,
    std::set<int>& visited)
{
    if (depth > SplitViewerCoreConfig::kMaximumTreeDepth || id < 0 || visited.size() >= static_cast<size_t>(SplitViewerCoreConfig::kMaximumNodes) || !visited.insert(id).second)
    {
        return nullptr;
    }
    const std::wstring section = prefix + L"Node" + CStringManager::Format(L"%d", id);
    const std::wstring kind = getValue(profile, section, L"Kind", L"");
    if (kind != L"Leaf" && kind != L"leaf" && kind != L"Split" && kind != L"split")
    {
        return nullptr;
    }
    std::unique_ptr<SplitViewerCoreNode> node(new SplitViewerCoreNode());
    if (kind == L"Split" || kind == L"split")
    {
        node->kind = SPLITVIEWER_CORE_NODE_SPLIT;
        node->direction = CStringManager::parseInt32(
            getValue(profile, section, L"Direction", L"0"), 0) == 0 ?
            SPLITVIEWER_CORE_SPLIT_HORIZONTAL : SPLITVIEWER_CORE_SPLIT_VERTICAL;
        node->ratio = SplitViewerCoreGeometryHelper::clampDouble(CStringManager::parseFiniteDouble(
            getValue(profile, section, L"Ratio", formatDouble(SplitViewerCoreConfig::kDefaultSplitRatio)), SplitViewerCoreConfig::kDefaultSplitRatio), SplitViewerCoreConfig::kMinimumSplitRatio, SplitViewerCoreConfig::kMaximumSplitRatio);
        node->first = loadNode(profile, prefix, CStringManager::parseInt32(
            getValue(profile, section, L"First", L"-1"), -1), depth + 1, visited);
        node->second = loadNode(profile, prefix, CStringManager::parseInt32(
            getValue(profile, section, L"Second", L"-1"), -1), depth + 1, visited);
        if (!node->first || !node->second)
        {
            return nullptr;
        }
        return node.release();
    }

    node->view.path = getValue(profile, section, L"Path", L"");
    node->view.hasImage = CStringManager::parseInt32(
        getValue(profile, section, L"HasImage", L"0"), 0) != 0;
    node->view.autoFit = CStringManager::parseInt32(
        getValue(profile, section, L"AutoFit", L"1"), 1) != 0;
    node->view.scale = CStringManager::parseFiniteDouble(
        getValue(profile, section, L"Scale", L"1"), 0.0);
    if (node->view.scale <= 0.0)
    {
        return nullptr;
    }
    node->view.offsetX = CStringManager::parseFiniteDouble(
        getValue(profile, section, L"OffsetX", L"0"), 0.0);
    node->view.offsetY = CStringManager::parseFiniteDouble(
        getValue(profile, section, L"OffsetY", L"0"), 0.0);
    const int contentKind = CStringManager::parseInt32(getValue(profile, section, L"ContentKind",
        node->view.hasImage ? L"1" : L"0"), node->view.hasImage ? 1 : 0);
    node->view.contentKind = static_cast<SplitViewerCoreContentKind>((std::max)(0, (std::min)(2, contentKind)));
    if (!node->view.hasImage && node->view.contentKind == SPLITVIEWER_CORE_CONTENT_IMAGE)
    {
        node->view.contentKind = SPLITVIEWER_CORE_CONTENT_EMPTY;
    }
    return node.release();
}

void SplitViewerCoreProfileHelper::writeProfileText(const SplitViewerCoreProfile& profile, std::wstring& text)
{
    text.clear();
    for (std::map<std::wstring, std::map<std::wstring, std::wstring> >::const_iterator sectionIt = profile.values.begin();
        sectionIt != profile.values.end();
        ++sectionIt)
    {
        text += L"[";
        text += sectionIt->first;
        text += L"]\r\n";
        for (std::map<std::wstring, std::wstring>::const_iterator valueIt = sectionIt->second.begin();
            valueIt != sectionIt->second.end();
            ++valueIt)
        {
            text += valueIt->first;
            text += L"=";
            text += valueIt->second;
            text += L"\r\n";
        }
        text += L"\r\n";
    }
}

bool SplitViewerCoreProfileHelper::parseProfileText(const std::vector<uint8_t>& bytes,
    SplitViewerCoreProfile& profile)
{
    profile.values.clear();
    if (bytes.size() < 2 || bytes.size() > SplitViewerCoreConfig::kMaximumProfileBytes)
    {
        return false;
    }
    std::wstring text;
    if (!CStringManager::utf16LeToWide(bytes, text))
    {
        return false;
    }

    std::wstring section;
    size_t lineStart = 0;
    while (lineStart <= text.size())
    {
        size_t lineEnd = text.find(L'\n', lineStart);
        if (lineEnd == std::wstring::npos)
        {
            lineEnd = text.size();
        }
        std::wstring line = text.substr(lineStart, lineEnd - lineStart);
        if (!line.empty() && line[line.size() - 1] == L'\r')
        {
            line.erase(line.size() - 1);
        }
        if (!line.empty() && line[0] == L'[' && line[line.size() - 1] == L']')
        {
            section = line.substr(1, line.size() - 2);
        }
        else
        {
            const size_t equal = line.find(L'=');
            if (!section.empty() && equal != std::wstring::npos)
            {
                setValue(profile, section, line.substr(0, equal), line.substr(equal + 1));
            }
        }
        if (lineEnd == text.size())
        {
            break;
        }
        lineStart = lineEnd + 1;
    }
    return !profile.values.empty();
}

bool SplitViewerCoreProfileHelper::serializeProfile(const SplitViewerCoreDocument& document, std::vector<uint8_t>& bytes)
{
    if (document.layerCount() > SplitViewerCoreConfig::kMaximumLayers)
    {
        return false;
    }
    SplitViewerCoreProfile profile;
    setValue(profile, L"SplitViewer", L"Version", L"2");
    setValue(profile, L"SplitViewer", L"StageAspect", formatDouble(document.stageAspect()));
    setValue(profile, L"SplitViewer", L"BorderVisible", CStringManager::Format(L"%d", document.borderVisible() ? 1 : 0));
    setValue(profile, L"SplitViewer", L"SelectedLayer", CStringManager::Format(L"%d", document.selectedLayer()));
    setValue(profile, L"SplitViewer", L"LayerCount", CStringManager::Format(L"%d", document.layerCount()));
    setValue(profile, L"Window", L"Left", CStringManager::Format(L"%d", document.windowLeft()));
    setValue(profile, L"Window", L"Top", CStringManager::Format(L"%d", document.windowTop()));
    setValue(profile, L"Window", L"Right", CStringManager::Format(L"%d", document.windowRight()));
    setValue(profile, L"Window", L"Bottom", CStringManager::Format(L"%d", document.windowBottom()));

    int baseNextId = 0;
    setValue(profile, L"Base", L"Root", CStringManager::Format(L"%d", baseNextId));
    if (!saveNode(profile, L"Base", document.baseRoot(), baseNextId))
    {
        return false;
    }
    setValue(profile, L"Base", L"NodeCount", CStringManager::Format(L"%d", baseNextId));
    for (int i = 0; i < document.layerCount(); ++i)
    {
        const SplitViewerCoreLayer* layer = document.layerAt(i);
        const std::wstring section = L"Layer" + CStringManager::Format(L"%d", i);
        setValue(profile, section, L"Left", formatDouble(layer ? layer->rect.left : SplitViewerCoreConfig::kDefaultLayerStart));
        setValue(profile, section, L"Top", formatDouble(layer ? layer->rect.top : SplitViewerCoreConfig::kDefaultLayerStart));
        setValue(profile, section, L"Right", formatDouble(layer ? layer->rect.right : SplitViewerCoreConfig::kDefaultLayerEnd));
        setValue(profile, section, L"Bottom", formatDouble(layer ? layer->rect.bottom : SplitViewerCoreConfig::kDefaultLayerEnd));
        int layerNextId = 0;
        setValue(profile, section, L"Root", CStringManager::Format(L"%d", layerNextId));
        if (!saveNode(profile, section + L"_", layer ? layer->root : nullptr, layerNextId))
        {
            return false;
        }
        setValue(profile, section, L"NodeCount", CStringManager::Format(L"%d", layerNextId));
    }

    std::wstring text;
    writeProfileText(profile, text);
    std::vector<uint8_t> encoded;
    if (!CStringManager::wideToUtf16Le(text, encoded) ||
        encoded.size() > static_cast<size_t>(SplitViewerCoreConfig::kMaximumProfileBytes))
    {
        return false;
    }
    bytes.swap(encoded);
    return true;
}

bool SplitViewerCoreProfileHelper::deserializeProfile(const std::vector<uint8_t>& bytes, SplitViewerCoreDocument& document)
{
    SplitViewerCoreProfile profile;
    if (!parseProfileText(bytes, profile))
    {
        return false;
    }
    if (getValue(profile,L"SplitViewer",L"Version",L"") != L"2")
    {
        return false;
    }
    std::set<int> baseVisited;
    std::unique_ptr<SplitViewerCoreNode> base(loadNode(profile, L"Base",
        CStringManager::parseInt32(getValue(profile, L"Base", L"Root", L"-1"), -1), 0, baseVisited));
    if (!base)
    {
        return false;
    }

    SplitViewerCoreDocument loaded;
    loaded.setBaseRoot(base.release());
    const int layerCount = CStringManager::parseInt32(getValue(profile, L"SplitViewer", L"LayerCount", L"0"), -1);
    // Reject invalid counts instead of silently truncating saved content.
    if (layerCount < 0 || layerCount > SplitViewerCoreConfig::kMaximumLayers)
    {
        return false;
    }
    for (int i = 0; i < layerCount; ++i)
    {
        std::unique_ptr<SplitViewerCoreLayer> layer(new SplitViewerCoreLayer());
        const std::wstring section = L"Layer" + CStringManager::Format(L"%d", i);
        layer->rect.left = SplitViewerCoreGeometryHelper::clampDouble(CStringManager::parseFiniteDouble(
            getValue(profile, section, L"Left", formatDouble(SplitViewerCoreConfig::kDefaultLayerStart)), SplitViewerCoreConfig::kDefaultLayerStart), 0.0, 1.0);
        layer->rect.top = SplitViewerCoreGeometryHelper::clampDouble(CStringManager::parseFiniteDouble(
            getValue(profile, section, L"Top", formatDouble(SplitViewerCoreConfig::kDefaultLayerStart)), SplitViewerCoreConfig::kDefaultLayerStart), 0.0, 1.0);
        layer->rect.right = SplitViewerCoreGeometryHelper::clampDouble(CStringManager::parseFiniteDouble(
            getValue(profile, section, L"Right", formatDouble(SplitViewerCoreConfig::kDefaultLayerEnd)), SplitViewerCoreConfig::kDefaultLayerEnd), 0.0, 1.0);
        layer->rect.bottom = SplitViewerCoreGeometryHelper::clampDouble(CStringManager::parseFiniteDouble(
            getValue(profile, section, L"Bottom", formatDouble(SplitViewerCoreConfig::kDefaultLayerEnd)), SplitViewerCoreConfig::kDefaultLayerEnd), 0.0, 1.0);
        SplitViewerCoreGeometryHelper::constrainLayerRect(layer->rect, SplitViewerCoreRect(0.0, 0.0, 1.0, 1.0), SplitViewerCoreConfig::kProfileMinimumLayerWidth, SplitViewerCoreConfig::kProfileMinimumLayerHeight);
        std::set<int> visited;
        std::unique_ptr<SplitViewerCoreNode> root(loadNode(profile, section + L"_",
            CStringManager::parseInt32(getValue(profile, section, L"Root", L"-1"), -1), 0, visited));
        if (!root)
        {
            return false;
        }
        delete layer->root;
        layer->root = root.release();
        loaded.appendLayer(layer.get());
        layer.release();
    }
    loaded.setSelectedLayer(CStringManager::parseInt32(
        getValue(profile, L"SplitViewer", L"SelectedLayer", L"-1"), -1));
    loaded.setBorderVisible(CStringManager::parseInt32(
        getValue(profile, L"SplitViewer", L"BorderVisible", L"1"), 1) != 0);
    loaded.setStageAspect(CStringManager::parseFiniteDouble(
        getValue(profile, L"SplitViewer", L"StageAspect", formatDouble(SplitViewerCoreConfig::kDefaultStageAspect)), SplitViewerCoreConfig::kDefaultStageAspect));
    loaded.setWindowRect(
        CStringManager::parseInt32(getValue(profile, L"Window", L"Left", L"-1"), -1),
        CStringManager::parseInt32(getValue(profile, L"Window", L"Top", L"-1"), -1),
        CStringManager::parseInt32(getValue(profile, L"Window", L"Right", L"-1"), -1),
        CStringManager::parseInt32(getValue(profile, L"Window", L"Bottom", L"-1"), -1));
    document.swap(loaded);
    return true;
}