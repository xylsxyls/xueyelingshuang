#include "SplitViewerCorePackageHelper.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreFormat.h"
#include <cstring>

void SplitViewerCorePackageHelper::appendUInt32(std::vector<uint8_t>& bytes, uint32_t value)
{
    bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>(value & 0xFF));
}

uint32_t SplitViewerCorePackageHelper::readUInt32(const std::vector<uint8_t>& bytes, size_t index)
{
    return (static_cast<uint32_t>(bytes[index]) << 24) | (static_cast<uint32_t>(bytes[index + 1]) << 16) |
        (static_cast<uint32_t>(bytes[index + 2]) << 8) | static_cast<uint32_t>(bytes[index + 3]);
}

uint32_t SplitViewerCorePackageHelper::crc32(uint32_t crc, const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (int32_t bit = 0; bit < 8; ++bit)
        {
            crc = (crc & 1) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);
        }
    }
    return crc;
}

uint32_t SplitViewerCorePackageHelper::chunkCrc(const uint8_t* type, const uint8_t* data, size_t size)
{
    uint32_t crc = crc32(0xFFFFFFFFu, type, 4);
    if (data && size > 0)
    {
        crc = crc32(crc, data, size);
    }
    return crc ^ 0xFFFFFFFFu;
}

bool SplitViewerCorePackageHelper::startsWithPng(const std::vector<uint8_t>& bytes)
{
    return bytes.size() >= sizeof(SplitViewerCoreFormat::kPngSignature) &&
        std::memcmp(&bytes[0], SplitViewerCoreFormat::kPngSignature, sizeof(SplitViewerCoreFormat::kPngSignature)) == 0;
}

bool SplitViewerCorePackageHelper::appendChunk(
    std::vector<uint8_t>& bytes, const uint8_t* type, const std::vector<uint8_t>& data)
{
    if (!type || data.size() > 0xFFFFFFFFu)
    {
        return false;
    }
    appendUInt32(bytes, static_cast<uint32_t>(data.size()));
    bytes.insert(bytes.end(), type, type + 4);
    bytes.insert(bytes.end(), data.begin(), data.end());
    appendUInt32(bytes, chunkCrc(type, data.empty() ? nullptr : &data[0], data.size()));
    return true;
}

bool SplitViewerCorePackageHelper::buildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes, std::vector<uint8_t>& packageBytes, const SplitViewerCoreConfig& config)
{
    if (!config.isValid() || !startsWithPng(thumbnailPng) ||
        configBytes.size() > static_cast<size_t>(config.m_maximumProfileBytes))
    {
        return false;
    }
    size_t position = sizeof(SplitViewerCoreFormat::kPngSignature);
    while (thumbnailPng.size() - position >= 12)
    {
        const size_t chunkStart = position;
        const uint32_t length = readUInt32(thumbnailPng, position);
        const size_t remaining = thumbnailPng.size() - chunkStart;
        if (length > remaining - 12)
        {
            return false;
        }
        if (std::memcmp(&thumbnailPng[chunkStart + 4], SplitViewerCoreFormat::kPngIend, 4) == 0)
        {
            if (length != 0)
            {
                return false;
            }
            std::vector<uint8_t> result(thumbnailPng.begin(), thumbnailPng.begin() + chunkStart);
            if (!appendChunk(result, SplitViewerCoreFormat::kConfigChunk, configBytes))
            {
                return false;
            }
            result.insert(result.end(), thumbnailPng.begin() + chunkStart, thumbnailPng.end());
            packageBytes.swap(result);
            return true;
        }
        position += static_cast<size_t>(length) + 12;
    }
    return false;
}

bool SplitViewerCorePackageHelper::extractEmbeddedConfig(
    const std::vector<uint8_t>& bytes, std::vector<uint8_t>& configBytes, const SplitViewerCoreConfig& config)
{
    if (!config.isValid())
    {
        return false;
    }
    if (startsWithPng(bytes))
    {
        size_t position = sizeof(SplitViewerCoreFormat::kPngSignature);
        while (bytes.size() - position >= 12)
        {
            const size_t chunkStart = position;
            const uint32_t length = readUInt32(bytes, position);
            const size_t remaining = bytes.size() - chunkStart;
            if (length > remaining - 12)
            {
                return false;
            }
            const uint8_t* type = &bytes[chunkStart + 4];
            if (std::memcmp(type, SplitViewerCoreFormat::kConfigChunk, 4) == 0)
            {
                if (length > static_cast<uint32_t>(config.m_maximumProfileBytes))
                {
                    return false;
                }
                const size_t dataStart = chunkStart + 8;
                if (readUInt32(bytes, dataStart + length) != chunkCrc(type, &bytes[dataStart], length))
                {
                    return false;
                }
                std::vector<uint8_t> result(bytes.begin() + dataStart, bytes.begin() + dataStart + length);
                configBytes.swap(result);
                return true;
            }
            if (std::memcmp(type, SplitViewerCoreFormat::kPngIend, 4) == 0)
            {
                break;
            }
            position += static_cast<size_t>(length) + 12;
        }
    }

    const uint8_t* marker = reinterpret_cast<const uint8_t*>(SplitViewerCoreFormat::kLegacyMarker);
    const size_t markerSize = sizeof(SplitViewerCoreFormat::kLegacyMarker) - 1;
    if (bytes.size() >= markerSize)
    {
        for (size_t i = 0; i <= bytes.size() - markerSize; ++i)
        {
            if (std::memcmp(&bytes[i], marker, markerSize) == 0 && i + markerSize < bytes.size())
            {
                if (bytes.size() - i - markerSize > static_cast<size_t>(config.m_maximumProfileBytes))
                {
                    return false;
                }
                std::vector<uint8_t> result(bytes.begin() + i + markerSize, bytes.end());
                configBytes.swap(result);
                return true;
            }
        }
    }
    return false;
}