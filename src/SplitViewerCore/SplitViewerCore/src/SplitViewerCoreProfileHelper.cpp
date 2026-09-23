#include "SplitViewerCoreProfileHelper.h"
#include "SplitViewerCore.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreGeometryHelper.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>
#include <locale>

std::wstring SplitViewerCoreProfileHelper::formatInt(int value)
{
    std::wostringstream stream;
    stream.imbue(std::locale::classic());
    stream << value;
    return stream.str();
}

std::wstring SplitViewerCoreProfileHelper::formatDouble(double value)
{
    std::wostringstream stream;
    stream.imbue(std::locale::classic());
    stream.setf(std::ios::fixed);
    stream.precision(10);
    stream << value;
    return stream.str();
}

int SplitViewerCoreProfileHelper::parseInt(const std::wstring& value, int defaultValue)
{
    std::wistringstream stream(value);
    stream.imbue(std::locale::classic());
    int result = defaultValue;
    stream >> result;
    return stream.fail() ? defaultValue : result;
}

double SplitViewerCoreProfileHelper::parseDouble(const std::wstring& value, double defaultValue)
{
    std::wistringstream stream(value);
    stream.imbue(std::locale::classic());
    double result = defaultValue;
    stream >> result;
    return stream.fail() || !std::isfinite(result) ? defaultValue : result;
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

void SplitViewerCoreProfileHelper::saveNode(SplitViewerCoreProfile& profile,
    const std::wstring& prefix,
    const SplitViewerCoreNode* node,
    int& nextId)
{
    const int id = nextId++;
    const std::wstring section = prefix + L"Node" + formatInt(id);
    if (!node || node->isLeaf())
    {
        setValue(profile, section, L"Kind", L"Leaf");
        if (node)
        {
            setValue(profile, section, L"Path", node->view.path);
            setValue(profile, section, L"HasImage", formatInt(node->view.hasImage ? 1 : 0));
            setValue(profile, section, L"AutoFit", formatInt(node->view.autoFit ? 1 : 0));
            setValue(profile, section, L"Scale", formatDouble(node->view.scale));
            setValue(profile, section, L"OffsetX", formatDouble(node->view.offsetX));
            setValue(profile, section, L"OffsetY", formatDouble(node->view.offsetY));
            setValue(profile, section, L"ContentKind", formatInt(static_cast<int>(node->view.contentKind)));
        }
        return;
    }

    setValue(profile, section, L"Kind", L"Split");
    setValue(profile, section, L"Direction",
        formatInt(node->direction == SPLITVIEWER_CORE_SPLIT_HORIZONTAL ? 0 : 1));
    setValue(profile, section, L"Ratio", formatDouble(node->ratio));
    const int firstId = nextId;
    saveNode(profile, prefix, node->first, nextId);
    const int secondId = nextId;
    saveNode(profile, prefix, node->second, nextId);
    setValue(profile, section, L"First", formatInt(firstId));
    setValue(profile, section, L"Second", formatInt(secondId));
}

SplitViewerCoreNode* SplitViewerCoreProfileHelper::loadNode(const SplitViewerCoreProfile& profile,
    const std::wstring& prefix,
    int id,
    int depth,
    std::set<int>& visited)
{
    if (depth > 128 || id < 0 || visited.size() >= 4096 || !visited.insert(id).second)
    {
        return NULL;
    }
    const std::wstring section = prefix + L"Node" + formatInt(id);
    const std::wstring kind = getValue(profile, section, L"Kind", L"");
    if (kind != L"Leaf" && kind != L"leaf" && kind != L"Split" && kind != L"split") return nullptr;
    SplitViewerCoreNode* node = new SplitViewerCoreNode();
    if (kind == L"Split" || kind == L"split")
    {
        node->kind = SPLITVIEWER_CORE_NODE_SPLIT;
        node->direction = parseInt(
            getValue(profile, section, L"Direction", L"0"), 0) == 0 ?
            SPLITVIEWER_CORE_SPLIT_HORIZONTAL : SPLITVIEWER_CORE_SPLIT_VERTICAL;
        node->ratio = SplitViewerCoreGeometryHelper::clampDouble(parseDouble(
            getValue(profile, section, L"Ratio", L"0.5"), 0.5), 0.02, 0.98);
        delete node->first;
        delete node->second;
        node->first = loadNode(profile, prefix, parseInt(
            getValue(profile, section, L"First", L"-1"), -1), depth + 1, visited);
        node->second = loadNode(profile, prefix, parseInt(
            getValue(profile, section, L"Second", L"-1"), -1), depth + 1, visited);
        if (!node->first || !node->second)
        {
            delete node;
            return nullptr;
        }
        return node;
    }

    node->view.path = getValue(profile, section, L"Path", L"");
    node->view.hasImage = parseInt(
        getValue(profile, section, L"HasImage", L"0"), 0) != 0;
    node->view.autoFit = parseInt(
        getValue(profile, section, L"AutoFit", L"1"), 1) != 0;
    node->view.scale = parseDouble(
        getValue(profile, section, L"Scale", L"1"), 1.0);
    node->view.offsetX = parseDouble(
        getValue(profile, section, L"OffsetX", L"0"), 0.0);
    node->view.offsetY = parseDouble(
        getValue(profile, section, L"OffsetY", L"0"), 0.0);
    const int contentKind = parseInt(getValue(profile, section, L"ContentKind",
        node->view.hasImage ? L"1" : L"0"), node->view.hasImage ? 1 : 0);
    node->view.contentKind = static_cast<SplitViewerCoreContentKind>((std::max)(0, (std::min)(2, contentKind)));
    if (!node->view.hasImage && node->view.contentKind == SPLITVIEWER_CORE_CONTENT_IMAGE)
    {
        node->view.contentKind = SPLITVIEWER_CORE_CONTENT_EMPTY;
    }
    return node;
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
    if (bytes.size() < 2 || bytes.size() > 16*1024*1024)
    {
        return false;
    }
    const size_t offset = bytes[0] == 0xFF && bytes[1] == 0xFE ? 2 : 0;
    if (((bytes.size() - offset) & 1) != 0)
    {
        return false;
    }
    std::wstring text;
    text.reserve((bytes.size() - offset) / 2);
    for (size_t i = offset; i + 1 < bytes.size(); i += 2)
    {
        uint32_t value=bytes[i] | (static_cast<uint16_t>(bytes[i+1])<<8);
        if (sizeof(wchar_t)>2 && value>=0xD800 && value<=0xDBFF)
        {
            if (i+3>=bytes.size()) return false;
            const uint32_t low=bytes[i+2] | (static_cast<uint16_t>(bytes[i+3])<<8);
            if (low<0xDC00 || low>0xDFFF) return false;
            value=0x10000+((value-0xD800)<<10)+(low-0xDC00);
            i+=2;
        }
        text.push_back(static_cast<wchar_t>(value));
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
    SplitViewerCoreProfile profile;
    setValue(profile, L"SplitViewer", L"Version", L"2");
    setValue(profile, L"SplitViewer", L"StageAspect", formatDouble(document.stageAspect()));
    setValue(profile, L"SplitViewer", L"BorderVisible", formatInt(document.borderVisible() ? 1 : 0));
    setValue(profile, L"SplitViewer", L"SelectedLayer", formatInt(document.selectedLayer()));
    setValue(profile, L"SplitViewer", L"LayerCount", formatInt(document.layerCount()));
    setValue(profile, L"Window", L"Left", formatInt(document.windowLeft()));
    setValue(profile, L"Window", L"Top", formatInt(document.windowTop()));
    setValue(profile, L"Window", L"Right", formatInt(document.windowRight()));
    setValue(profile, L"Window", L"Bottom", formatInt(document.windowBottom()));

    int baseNextId = 0;
    setValue(profile, L"Base", L"Root", formatInt(baseNextId));
    saveNode(profile, L"Base", document.baseRoot(), baseNextId);
    setValue(profile, L"Base", L"NodeCount", formatInt(baseNextId));
    for (int i = 0; i < document.layerCount(); ++i)
    {
        const SplitViewerCoreLayer* layer = document.layerAt(i);
        const std::wstring section = L"Layer" + formatInt(i);
        setValue(profile, section, L"Left", formatDouble(layer ? layer->rect.left : 0.25));
        setValue(profile, section, L"Top", formatDouble(layer ? layer->rect.top : 0.25));
        setValue(profile, section, L"Right", formatDouble(layer ? layer->rect.right : 0.75));
        setValue(profile, section, L"Bottom", formatDouble(layer ? layer->rect.bottom : 0.75));
        int layerNextId = 0;
        setValue(profile, section, L"Root", formatInt(layerNextId));
        saveNode(profile, section + L"_", layer ? layer->root : NULL, layerNextId);
        setValue(profile, section, L"NodeCount", formatInt(layerNextId));
    }

    std::wstring text;
    writeProfileText(profile, text);
    bytes.clear();
    bytes.push_back(0xFF);
    bytes.push_back(0xFE);
    for (size_t i = 0; i < text.size(); ++i)
    {
        uint32_t scalar=static_cast<uint32_t>(text[i]);
        if (sizeof(wchar_t)>2 && scalar>0xFFFF)
        {
            if (scalar>0x10FFFF) return false;
            scalar-=0x10000;
            const uint16_t high=static_cast<uint16_t>(0xD800+(scalar>>10));
            bytes.push_back(static_cast<uint8_t>(high & 0xFF));
            bytes.push_back(static_cast<uint8_t>(high >> 8));
            scalar=0xDC00+(scalar & 0x3FF);
        }
        const uint16_t value = static_cast<uint16_t>(scalar);
        bytes.push_back(static_cast<uint8_t>(value & 0xFF));
        bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    }
    return !bytes.empty();
}

bool SplitViewerCoreProfileHelper::deserializeProfile(const std::vector<uint8_t>& bytes, SplitViewerCoreDocument& document)
{
    SplitViewerCoreProfile profile;
    if (!parseProfileText(bytes, profile))
    {
        return false;
    }
    if (getValue(profile,L"SplitViewer",L"Version",L"") != L"2") return false;
    std::set<int> baseVisited;
    SplitViewerCoreNode* base = loadNode(profile, L"Base",
        parseInt(getValue(profile, L"Base", L"Root", L"-1"), -1), 0, baseVisited);
    if (!base)
    {
        return false;
    }

    SplitViewerCoreDocument loaded;
    loaded.setBaseRoot(base);
    const int layerCount = (std::max)(0, (std::min)(256, parseInt(
        getValue(profile, L"SplitViewer", L"LayerCount", L"0"), 0)));
    for (int i = 0; i < layerCount; ++i)
    {
        SplitViewerCoreLayer* layer = new SplitViewerCoreLayer();
        const std::wstring section = L"Layer" + formatInt(i);
        layer->rect.left = SplitViewerCoreGeometryHelper::clampDouble(parseDouble(
            getValue(profile, section, L"Left", L"0.25"), 0.25), 0.0, 1.0);
        layer->rect.top = SplitViewerCoreGeometryHelper::clampDouble(parseDouble(
            getValue(profile, section, L"Top", L"0.25"), 0.25), 0.0, 1.0);
        layer->rect.right = SplitViewerCoreGeometryHelper::clampDouble(parseDouble(
            getValue(profile, section, L"Right", L"0.75"), 0.75), 0.0, 1.0);
        layer->rect.bottom = SplitViewerCoreGeometryHelper::clampDouble(parseDouble(
            getValue(profile, section, L"Bottom", L"0.75"), 0.75), 0.0, 1.0);
        SplitViewerCoreGeometryHelper::constrainLayerRect(layer->rect, SplitViewerCoreRect(0.0, 0.0, 1.0, 1.0), 0.12, 0.10);
        delete layer->root;
        std::set<int> visited;
        layer->root = loadNode(profile, section + L"_",
            parseInt(getValue(profile, section, L"Root", L"-1"), -1), 0, visited);
        if (!layer->root)
        {
            delete layer;
            return false;
        }
        loaded.appendLayer(layer);
    }
    loaded.setSelectedLayer(parseInt(
        getValue(profile, L"SplitViewer", L"SelectedLayer", L"-1"), -1));
    loaded.setBorderVisible(parseInt(
        getValue(profile, L"SplitViewer", L"BorderVisible", L"1"), 1) != 0);
    loaded.setStageAspect(parseDouble(
        getValue(profile, L"SplitViewer", L"StageAspect", L"1.3333333333"), 4.0 / 3.0));
    loaded.setWindowRect(
        parseInt(getValue(profile, L"Window", L"Left", L"-1"), -1),
        parseInt(getValue(profile, L"Window", L"Top", L"-1"), -1),
        parseInt(getValue(profile, L"Window", L"Right", L"-1"), -1),
        parseInt(getValue(profile, L"Window", L"Bottom", L"-1"), -1));
    document.swap(loaded);
    return true;
}