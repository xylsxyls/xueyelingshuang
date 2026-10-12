#include "AccountStore.h"
#include "Config.h"
#include "CookHelper.h"
#include "CookResourcePaths.h"
#include "CSystem/CSystemAPI.h"

bool AccountStore::publishVideo(const std::string &userId, const std::string &videoId, const std::string &caption,
    const std::vector<std::string> &recipeIds, VideoInfo &video, std::string &message, const std::string &coverImageId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount &account = ensureNoLock(userId);
    const std::string id = CookHelper::normalizeVideoId(videoId);
    const std::map<std::string, VideoInfo>::iterator found = m_videos.find(id);
    if (found == m_videos.end() || found->second.m_ownerUserId != userId || found->second.m_mediaType != "video" ||
        (found->second.m_status != "uploaded" && found->second.m_status != "published" &&
            found->second.m_status != "reviewing"))
    {
        message = "请先完成本人视频上传";
        return false;
    }
    const std::string file = resolveVideoFilePathNoLock(found->second.m_filePath);
    if (!CSystem::isPathWithin(file, CookResourcePaths::path(g_config.m_videoStorageDirectoryName)) ||
        !CookHelper::isSupportedVideoFile(file))
    {
        message = "上传视频文件不可用";
        return false;
    }
    const Recipe *own = nullptr;
    for (size_t i = 0; i < found->second.m_recipeIds.size(); ++i)
    {
        const Recipe *linked = findCreationRecipeNoLock(userId, found->second.m_recipeIds[i]);
        if (linked != nullptr && m_recipeReviewOrder.count(linked->m_id) != 0 &&
            (recipeIds.size() != 1 || recipeIds[0] != linked->m_id))
        {
            message = "原始作品媒体请在创作界面修改，不能改成未审核的做菜分享";
            return false;
        }
    }
    for (size_t i = 0; i < recipeIds.size(); ++i)
    {
        const Recipe *recipe = findCreationRecipeNoLock(userId, recipeIds[i]);
        if (recipe != nullptr)
        {
            own = recipe;
        }
        else
        {
            recipe = findBaseRecipeNoLock(recipeIds[i]);
        }
        if (recipe == nullptr || !isRecipeOwned(*recipe, account))
        {
            message = "菜谱不存在或尚未购买";
            return false;
        }
    }
    if (own != nullptr)
    {
        if (recipeIds.size() != 1)
        {
            message = "原始作品请按一道菜提交，做菜分享可关联多道他人菜谱";
            return false;
        }
        Recipe candidate = *own;
        candidate.m_mediaIds.assign(1, id);
        if (!caption.empty())
        {
            candidate.m_subtitle = caption;
        }
        if (!coverImageId.empty())
        {
            candidate.m_coverImageId = coverImageId;
        }
        // 原始媒体变更与步骤/价格共用审核和额度，不能通过视频接口绕过。
        if (!submitCreationNoLock(userId, candidate, "", true, message))
        {
            return false;
        }
        video = m_videos[id];
        return true;
    }
    if (!coverImageId.empty() || found->second.m_status == "reviewing")
    {
        message = "审核中的原始作品请从创作界面提交";
        return false;
    }
    const VideoInfo before = found->second;
    found->second.m_caption = caption;
    found->second.m_recipeIds = recipeIds;
    found->second.m_status = "published";
    if (!saveNoLock(true))
    {
        found->second = before;
        saveNoLock(true);
        message = "发布未保存，请重试";
        return false;
    }
    video = found->second;
    message = "发布成功";
    return true;
}