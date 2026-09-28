#include "SplitViewerCoreConfig.h"

const uint8_t SplitViewerCoreConfig::PngSignature[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
const uint8_t SplitViewerCoreConfig::PngIend[4] = { 'I', 'E', 'N', 'D' };
const uint8_t SplitViewerCoreConfig::ConfigChunk[4] = { 's', 'v', 'C', 'f' };
const char SplitViewerCoreConfig::LegacyMarker[35] = "\r\n--SPLITVIEWER_CONFIG_UTF16LE--\r\n";

const double SplitViewerCoreConfig::kDefaultStageAspect = 4.0 / 3.0;
const double SplitViewerCoreConfig::kMinimumStageAspect = 0.1;
const double SplitViewerCoreConfig::kDefaultSplitRatio = 0.5;
const double SplitViewerCoreConfig::kMinimumSplitRatio = 0.02;
const double SplitViewerCoreConfig::kMaximumSplitRatio = 0.98;
const double SplitViewerCoreConfig::kLayerOrigin = 0.22;
const double SplitViewerCoreConfig::kLayerOffset = 0.03;
const int32_t SplitViewerCoreConfig::kLayerOffsetPeriod = 6;
const double SplitViewerCoreConfig::kLayerExtent = 0.46;
const double SplitViewerCoreConfig::kLayerOriginLimit = 0.72;
const double SplitViewerCoreConfig::kDefaultLayerStart = 0.25;
const double SplitViewerCoreConfig::kDefaultLayerEnd = 0.75;
const double SplitViewerCoreConfig::kProfileMinimumLayerWidth = 0.12;
const double SplitViewerCoreConfig::kProfileMinimumLayerHeight = 0.10;
const double SplitViewerCoreConfig::kZoomStep = 1.05;
const double SplitViewerCoreConfig::kFineZoomStep = 1.01;
const double SplitViewerCoreConfig::kMinimumScale = 0.0001;
const double SplitViewerCoreConfig::kMinimumFitScale = 0.05;
const double SplitViewerCoreConfig::kMaximumFitScale = 50.0;
const double SplitViewerCoreConfig::kMinimumZoomRange = 10.0;
const int32_t SplitViewerCoreConfig::kMaximumTreeDepth = 128;
const int32_t SplitViewerCoreConfig::kMaximumNodes = 4096;
const int32_t SplitViewerCoreConfig::kMaximumLayers = 256;
const int32_t SplitViewerCoreConfig::kMaximumProfileBytes = 16 * 1024 * 1024;