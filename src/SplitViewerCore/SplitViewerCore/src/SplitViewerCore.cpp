#include "SplitViewerCore.h"
#include "SplitViewerCoreGeometryHelper.h"
#include "SplitViewerCoreProfileHelper.h"
#include "SplitViewerCorePackageHelper.h"
#include "SplitViewerCoreMoveHelper.h"

void SplitViewerCoreSplitNodeRects(const SplitViewerCoreRect& owner, const SplitViewerCoreNode* node,
    SplitViewerCoreRect& first, SplitViewerCoreRect& splitter, SplitViewerCoreRect& second, double thickness)
{
    SplitViewerCoreGeometryHelper::splitNodeRects(owner, node, first, splitter, second, thickness);
}

void SplitViewerCoreConstrainLayerRect(
    SplitViewerCoreRect& rect, const SplitViewerCoreRect& bounds, double minWidth, double minHeight)
{
    SplitViewerCoreGeometryHelper::constrainLayerRect(rect, bounds, minWidth, minHeight);
}

double SplitViewerCoreFitScale(double imageWidth, double imageHeight, double contentWidth, double contentHeight)
{
    return SplitViewerCoreGeometryHelper::fitScale(imageWidth, imageHeight, contentWidth, contentHeight);
}

void SplitViewerCoreZoom(
    SplitViewerCoreLeafState& view, double fitScale, int32_t delta, bool fine, const SplitViewerCoreConfig& config)
{
    SplitViewerCoreGeometryHelper::zoom(view, fitScale, delta, fine, config);
}

void SplitViewerCoreResizeView(
    SplitViewerCoreLeafState& view, double oldWidth, double oldHeight, double newWidth, double newHeight)
{
    SplitViewerCoreGeometryHelper::resizeView(view, oldWidth, oldHeight, newWidth, newHeight);
}

bool SplitViewerCoreSerializeProfile(const SplitViewerCoreDocument& document, std::vector<uint8_t>& bytes)
{
    return SplitViewerCoreProfileHelper::serializeProfile(document, bytes);
}

bool SplitViewerCoreDeserializeProfile(const std::vector<uint8_t>& bytes, SplitViewerCoreDocument& document)
{
    return SplitViewerCoreProfileHelper::deserializeProfile(bytes, document);
}

bool SplitViewerCoreBuildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes, std::vector<uint8_t>& packageBytes)
{
    return SplitViewerCorePackageHelper::buildConfigPackage(
        thumbnailPng, configBytes, packageBytes, SplitViewerCoreConfig());
}

bool SplitViewerCoreExtractEmbeddedConfig(const std::vector<uint8_t>& bytes, std::vector<uint8_t>& configBytes)
{
    return SplitViewerCorePackageHelper::extractEmbeddedConfig(bytes, configBytes, SplitViewerCoreConfig());
}

bool SplitViewerCoreBuildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes, std::vector<uint8_t>& packageBytes, const SplitViewerCoreConfig& config)
{
    return SplitViewerCorePackageHelper::buildConfigPackage(thumbnailPng, configBytes, packageBytes, config);
}

bool SplitViewerCoreExtractEmbeddedConfig(
    const std::vector<uint8_t>& bytes, std::vector<uint8_t>& configBytes, const SplitViewerCoreConfig& config)
{
    return SplitViewerCorePackageHelper::extractEmbeddedConfig(bytes, configBytes, config);
}

SplitViewerCoreNode* SplitViewerCoreFindParent(SplitViewerCoreNode* root, SplitViewerCoreNode* target)
{
    if (!root || root->isLeaf() || !target)
    {
        return nullptr;
    }
    if (root->first == target || root->second == target)
    {
        return root;
    }
    SplitViewerCoreNode* parent = SplitViewerCoreFindParent(root->first, target);
    return parent ? parent : SplitViewerCoreFindParent(root->second, target);
}

SplitViewerCoreNode* SplitViewerCoreDirectionalTarget(
    SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreMoveDirection direction)
{
    return SplitViewerCoreMoveHelper::directionalTarget(root, source, direction);
}

bool SplitViewerCoreMoveContent(SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreNode* target)
{
    return SplitViewerCoreMoveHelper::moveContent(root, source, target);
}