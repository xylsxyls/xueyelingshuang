#include "CookResourcePaths.h"
#include "Config.h"
#include "CookHelper.h"
#include "CSystem/CSystemAPI.h"
#include <set>
#include <vector>

bool CookResourcePaths::relativeRoot(const std::string &relative, std::string &first)
{
    if (relative.empty() || relative.find('\0') != std::string::npos || relative.find(':') != std::string::npos ||
        relative[0] == '/' || relative[0] == '\\')
    {
        return false;
    }
    size_t begin = 0;
    while (begin < relative.size())
    {
        const size_t end = relative.find_first_of("\\/", begin);
        const std::string part = relative.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
        if (part.empty() || part == "." || part == "..")
        {
            return false;
        }
        if (begin == 0)
        {
            first = part;
        }
        if (end == std::string::npos)
        {
            return true;
        }
        begin = end + 1;
    }
    return false;
}

std::string CookResourcePaths::path(const std::string &relative)
{
    const std::string flat = flatRelative(relative);
    return !g_config.m_resourceRootPath.empty() && !flat.empty()
               ? CSystem::joinPath(g_config.m_resourceRootPath, flat)
               : "";
}

std::string CookResourcePaths::flatRelative(const std::string &relative)
{
    std::string first;
    if (!relativeRoot(relative, first))
    {
        return "";
    }
    std::string result = relative;
    for (size_t i = 0; i < result.size(); ++i)
    {
        if (result[i] == '\\')
        {
            result[i] = '/';
        }
    }
    if (CookHelper::lowerAscii(first) == "res")
    {
        if (result.size() <= 4)
        {
            return "";
        }
        result.erase(0, 4);
    }
    return result;
}

bool CookResourcePaths::prepare(std::string &message)
{
    const std::string exe = CSystem::canonicalPath(CSystem::GetCurrentExePath(), true);
    std::string configured = g_config.m_resourceRootPath;
    if (!configured.empty() && configured[0] != '/' && configured[0] != '\\' &&
        configured.find(':') == std::string::npos)
    {
        configured = CSystem::joinPath(exe, configured);
    }
    std::string root = CSystem::canonicalPath(configured, false);
    if (root.empty() || root.back() == '\\' || root.back() == '/' || root == exe ||
        CSystem::isPathWithin(root, exe, false) || CSystem::isPathWithin(exe, root, false))
    {
        message = "Storage.ResourceRoot must be a separate directory outside the program";
        return false;
    }
    if (!CSystem::createDirectories(root))
    {
        message = "Cannot create external resource root: " + root;
        return false;
    }
    root = CSystem::canonicalPath(root, true);
    if (root.empty() || CSystem::isPathWithin(root, exe, true) || CSystem::isPathWithin(exe, root, true))
    {
        message = "Resolved resource root overlaps the program";
        return false;
    }
    g_config.m_resourceRootPath = root;
    std::string *directories[] = {&g_config.m_recipeResourceDirectoryName, &g_config.m_videoResourceDirectoryName,
        &g_config.m_videoStorageDirectoryName, &g_config.m_videoSegmentCacheDirectoryName, &g_config.m_avatarDirectoryName,
        &g_config.m_voiceCacheDirectoryName};
    std::set<std::string> roots;
    std::set<std::string> legacyRoots;
    legacyRoots.insert("recipes");
    legacyRoots.insert("video");
    for (size_t i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i)
    {
        std::string first;
        const std::string flat = flatRelative(*directories[i]);
        if (flat.empty() || !relativeRoot(flat, first))
        {
            message = "Resource subdirectory must be a safe relative path: " + *directories[i];
            return false;
        }
        if (flat != *directories[i] && CookHelper::lowerAscii(directories[i]->substr(0, 3)) == "res")
        {
            legacyRoots.insert(first);
        }
        *directories[i] = flat;
        roots.insert(first);
    }
    // 兼容程序目录res以及前一版外部根res；每个子目录独立校验迁移，避免根目录重叠。
    const std::string bases[] = {exe, root};
    for (size_t i = 0; i < sizeof(bases) / sizeof(bases[0]); ++i)
    {
        const std::string legacyBase = CSystem::joinPath(bases[i], "res");
        if (CSystem::DirOrFileExist(legacyBase) &&
            CSystem::canonicalPath(legacyBase, true) != CSystem::canonicalPath(legacyBase, false))
        {
            message = "Legacy resource directory must not redirect through a link: " + legacyBase;
            return false;
        }
        for (std::set<std::string>::const_iterator it = legacyRoots.begin(); it != legacyRoots.end(); ++it)
        {
            if (!CSystem::migrateDirectory(CSystem::joinPath(bases[i], "res/" + *it), CSystem::joinPath(root, *it), message))
            {
                return false;
            }
        }
        // 只删除空res，保留未识别的历史资源，不递归删除。
        CSystem::DestroyDir(legacyBase);
    }
    for (std::set<std::string>::const_iterator it = roots.begin(); it != roots.end(); ++it)
    {
        if (!CSystem::migrateDirectory(CSystem::joinPath(exe, *it), CSystem::joinPath(root, *it), message))
        {
            return false;
        }
    }
    for (size_t i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i)
    {
        if (!CSystem::createDirectories(path(*directories[i])) ||
            !CSystem::isPathWithin(path(*directories[i]), root, true))
        {
            message = "Cannot create resource subdirectory: " + *directories[i];
            return false;
        }
    }
    message = "ok";
    return true;
}

std::string CookResourcePaths::mediaPath(const std::string &stored)
{
    std::string first;
    if (relativeRoot(stored, first))
    {
        return path(stored);
    }
    if (CSystem::isPathWithin(stored, g_config.m_resourceRootPath, true))
    {
        return stored;
    }
    // 旧快照可能记录旧EXE的绝对路径；仅转换已知媒体根下的原始后缀。
    std::string normalized = stored;
    for (size_t i = 0; i < normalized.size(); ++i)
    {
        if (normalized[i] == '\\')
        {
            normalized[i] = '/';
        }
    }
    const std::string folders[] = {g_config.m_videoStorageDirectoryName, g_config.m_videoResourceDirectoryName};
    for (size_t i = 0; i < sizeof(folders) / sizeof(folders[0]); ++i)
    {
        std::string marker = folders[i];
        for (size_t j = 0; j < marker.size(); ++j)
        {
            if (marker[j] == '\\')
            {
                marker[j] = '/';
            }
        }
        const size_t found = normalized.rfind("/" + marker + "/");
        if (found != std::string::npos)
        {
            return path(normalized.substr(found + 1));
        }
    }
    return "";
}