#include "CookAvatarHelper.h"
#include "CookResourcePaths.h"
#include "Config.h"
#include "CookHelper.h"
#include "CookRecipePresentation.h"
#include "CSystem/CSystemAPI.h"
#include <cstdio>
#include <fstream>

std::string CookAvatarHelper::root()
{
    return CookResourcePaths::path(g_config.m_avatarDirectoryName);
}

bool CookAvatarHelper::isKey(const std::string& key)
{
    if (key.size() != 36 || (key.substr(32) != ".png" && key.substr(32) != ".jpg"))
    {
        return false;
    }
    for (size_t i = 0; i < 32; ++i)
    {
        const char ch = key[i];
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
        {
            return false;
        }
    }
    return true;
}

bool CookAvatarHelper::save(const std::string& bytes, std::string& key, std::string& message)
{
    key.clear();
    if (bytes.empty() || bytes.size() > static_cast<size_t>(g_config.m_avatarMaxBytes))
    {
        message = "Avatar is empty or exceeds 512 KiB";
        return false;
    }
    const bool png = bytes.size() >= 8 && bytes.compare(0, 8, "\x89PNG\r\n\x1a\n", 8) == 0;
    const bool jpeg = bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xff &&
        static_cast<unsigned char>(bytes[1]) == 0xd8 && static_cast<unsigned char>(bytes[2]) == 0xff;
    if (!png && !jpeg)
    {
        message = "Avatar must be a PNG or JPEG image";
        return false;
    }
    const std::string nextKey = CSystem::uuid(0) + (png ? ".png" : ".jpg");
    const std::string directory = CookAvatarHelper::root();
    if (!isKey(nextKey) || !CookHelper::ensureDirectoryTree(directory))
    {
        message = "Cannot create avatar storage";
        return false;
    }
    const std::string path = CookHelper::appendPathSeparator(directory) + nextKey;
    // 先写独有暂存文件，验证成功后才公开最终文件名。
    const std::string temporary = path + ".part";
    std::ofstream output(temporary.c_str(), std::ios::binary | std::ios::trunc);
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.flush();
    const bool written = output.good();
    output.close();
    if (!written)
    {
        std::remove(temporary.c_str());
        message = "Cannot save avatar image";
        return false;
    }
    if (!CookRecipePresentation::IsSquareImage(temporary, static_cast<uint32_t>(g_config.m_avatarMaxSide)))
    {
        std::remove(temporary.c_str());
        message = "Avatar must be a valid square image, at most 1024 pixels";
        return false;
    }
    if (std::rename(temporary.c_str(), path.c_str()) != 0)
    {
        std::remove(temporary.c_str());
        message = "Cannot save avatar image";
        return false;
    }
    key = nextKey;
    message = "ok";
    return true;
}

bool CookAvatarHelper::read(const std::string& key, std::string& bytes, std::string& contentType)
{
    bytes.clear();
    if (!isKey(key))
    {
        return false;
    }
    const std::string path = CookHelper::appendPathSeparator(root()) + key;
    std::ifstream input(path.c_str(), std::ios::binary | std::ios::ate);
    const std::streamoff size = input.tellg();
    if (!input.good() || size <= 0 || size > g_config.m_avatarMaxBytes)
    {
        return false;
    }
    bytes.resize(static_cast<size_t>(size));
    input.seekg(0);
    input.read(&bytes[0], static_cast<std::streamsize>(size));
    if (!input.good())
    {
        bytes.clear();
        return false;
    }
    contentType = key.substr(32) == ".png" ? "image/png" : "image/jpeg";
    return true;
}

void CookAvatarHelper::remove(const std::string& key)
{
    if (isKey(key))
    {
        std::remove((CookHelper::appendPathSeparator(root()) + key).c_str());
    }
}