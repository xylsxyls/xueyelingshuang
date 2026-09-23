#include "SplitViewerCorePackageHelper.h"
#include "SplitViewerCoreConfig.h"
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
    return (static_cast<uint32_t>(bytes[index]) << 24) |
        (static_cast<uint32_t>(bytes[index + 1]) << 16) |
        (static_cast<uint32_t>(bytes[index + 2]) << 8) |
        static_cast<uint32_t>(bytes[index + 3]);
}

uint32_t SplitViewerCorePackageHelper::crc32(uint32_t crc, const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
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
    return bytes.size() >= sizeof(SplitViewerCoreConfig::PngSignature) &&
        std::memcmp(&bytes[0], SplitViewerCoreConfig::PngSignature, sizeof(SplitViewerCoreConfig::PngSignature)) == 0;
}

bool SplitViewerCorePackageHelper::appendChunk(std::vector<uint8_t>& bytes,
    const uint8_t* type,
    const std::vector<uint8_t>& data)
{
    if (!type || data.size() > 0xFFFFFFFFu)
    {
        return false;
    }
    appendUInt32(bytes, static_cast<uint32_t>(data.size()));
    bytes.insert(bytes.end(), type, type + 4);
    bytes.insert(bytes.end(), data.begin(), data.end());
    appendUInt32(bytes, chunkCrc(type,
        data.empty() ? NULL : &data[0], data.size()));
    return true;
}

bool SplitViewerCorePackageHelper::buildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes,
    std::vector<uint8_t>& packageBytes)
{
    packageBytes.clear();
    if (!startsWithPng(thumbnailPng))
    {
        return false;
    }
    size_t position = sizeof(SplitViewerCoreConfig::PngSignature);
    while (position + 12 <= thumbnailPng.size())
    {
        const size_t chunkStart = position;
        const uint32_t length = readUInt32(thumbnailPng, position);
        const size_t remaining = thumbnailPng.size() - chunkStart;
        if (length > remaining - 12)
        {
            return false;
        }
        if (std::memcmp(&thumbnailPng[chunkStart + 4], SplitViewerCoreConfig::PngIend, 4) == 0)
        {
            packageBytes.assign(thumbnailPng.begin(), thumbnailPng.begin() + chunkStart);
            if (!appendChunk(packageBytes, SplitViewerCoreConfig::ConfigChunk, configBytes))
            {
                packageBytes.clear();
                return false;
            }
            packageBytes.insert(packageBytes.end(), thumbnailPng.begin() + chunkStart, thumbnailPng.end());
            return true;
        }
        position += static_cast<size_t>(length) + 12;
    }
    return false;
}

bool SplitViewerCorePackageHelper::extractEmbeddedConfig(const std::vector<uint8_t>& bytes,
    std::vector<uint8_t>& configBytes)
{
    configBytes.clear();
    if (startsWithPng(bytes))
    {
        size_t position = sizeof(SplitViewerCoreConfig::PngSignature);
        while (position + 12 <= bytes.size())
        {
            const size_t chunkStart = position;
            const uint32_t length = readUInt32(bytes, position);
            const size_t remaining = bytes.size() - chunkStart;
            if (length > remaining - 12)
            {
                return false;
            }
            const uint8_t* type = &bytes[chunkStart + 4];
            if (std::memcmp(type, SplitViewerCoreConfig::ConfigChunk, 4) == 0)
            {
                const size_t dataStart = chunkStart + 8;
                if (readUInt32(bytes,dataStart+length) != chunkCrc(type,&bytes[dataStart],length)) return false;
                configBytes.assign(bytes.begin() + dataStart, bytes.begin() + dataStart + length);
                return true;
            }
            if (std::memcmp(type, SplitViewerCoreConfig::PngIend, 4) == 0)
            {
                break;
            }
            position += static_cast<size_t>(length) + 12;
        }
    }

    const uint8_t* marker = reinterpret_cast<const uint8_t*>(SplitViewerCoreConfig::LegacyMarker);
    const size_t markerSize = sizeof(SplitViewerCoreConfig::LegacyMarker) - 1;
    if (bytes.size() >= markerSize)
    {
        for (size_t i = 0; i <= bytes.size() - markerSize; ++i)
        {
            if (std::memcmp(&bytes[i], marker, markerSize) == 0 && i + markerSize < bytes.size())
            {
                configBytes.assign(bytes.begin() + i + markerSize, bytes.end());
                return true;
            }
        }
    }
    return false;
}