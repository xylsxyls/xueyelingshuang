#include "CookVideoUploadService.h"
#include "Config.h"
#include "CookHelper.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dirent.h>
#endif

namespace
{
static const int64_t kMaxChunkBytes = 512LL * 1024LL;
static const int64_t kSessionLifetimeSeconds = 60LL * 60LL;
static const size_t kMaxUploadSessions = 64;
static const size_t kMaxSessionsPerUser = 2;

int64_t nowSeconds()
{
	return static_cast<int64_t>(std::time(nullptr));
}

std::string makeUploadId(const std::string& userId)
{
	(void)userId;
	const std::string uuid = CSystem::uuid(0);
	std::string result;
	for (size_t i = 0; i < uuid.size(); ++i)
	{
		const unsigned char ch = static_cast<unsigned char>(uuid[i]);
		if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
		{
			result.push_back(static_cast<char>(ch));
		}
	}
	return result.size() >= 32 ? result : std::string();
}

std::string safeExtension(const std::string& fileName)
{
	std::string::size_type dot = fileName.find_last_of('.');
	if (dot == std::string::npos || dot + 1 >= fileName.size()) return std::string();
	std::string extension = fileName.substr(dot);
	for (size_t i = 0; i < extension.size(); ++i)
	{
		extension[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(extension[i])));
	}
	return CookHelper::isSupportedVideoFile("cook_upload" + extension) || extension == ".png" || extension == ".jpg" || extension == ".jpeg" ? extension : std::string();
}

std::string uploadRoot()
{
	return CookHelper::appendPathSeparator(CSystem::GetCurrentExePath()) + g_config.m_videoStorageDirectoryName;
}
}

CookVideoUploadService::CookVideoUploadService() :
m_stagingScanned(false)
{
}

void CookVideoUploadService::removeStalePartsNoLock(const std::string& root)
{
#ifdef _WIN32
	const std::string pattern = CookHelper::appendPathSeparator(root) + "*.part";
	WIN32_FIND_DATAA entry = {};
	HANDLE search = FindFirstFileA(pattern.c_str(), &entry);
	if (search == INVALID_HANDLE_VALUE) return;
	do
	{
		if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			std::remove((CookHelper::appendPathSeparator(root) + entry.cFileName).c_str());
		}
	} while (FindNextFileA(search, &entry));
	FindClose(search);
#else
	DIR* directory = opendir(root.c_str());
	if (directory == nullptr) return;
	struct dirent* entry = nullptr;
	while ((entry = readdir(directory)) != nullptr)
	{
		const std::string name(entry->d_name);
		if (name.size() > 5 && name.compare(name.size() - 5, 5, ".part") == 0)
		{
			std::remove((CookHelper::appendPathSeparator(root) + name).c_str());
		}
	}
	closedir(directory);
#endif
}

void CookVideoUploadService::pruneExpiredNoLock(int64_t now)
{
	for (std::map<std::string, Session>::iterator it = m_sessions.begin(); it != m_sessions.end();)
	{
		if (now - it->second.m_createdAtSeconds > kSessionLifetimeSeconds)
		{
			std::remove(it->second.m_tempPath.c_str());
			m_sessions.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

bool CookVideoUploadService::start(const std::string& userId,
                                   const std::string& mediaType,
                                   const std::string& fileName,
                                   int64_t expectedBytes,
                                   int64_t maxBytes,
                                   VideoUploadInfo& upload,
                                   std::string& message)
{
	if (userId.empty() || (mediaType != "video" && mediaType != "image") || expectedBytes <= 0 || maxBytes <= 0 ||
	    expectedBytes > maxBytes || expectedBytes > INT32_MAX)
	{
		message = "视频上传参数无效或文件超过限制";
		return false;
	}
    if (mediaType == "image" && expectedBytes > 4LL * 1024LL * 1024LL) { message = "Cover image exceeds 4 MiB"; return false; }
	const std::string extension = safeExtension(fileName);
	if (extension.empty() || (mediaType == "image" && extension != ".png" && extension != ".jpg" && extension != ".jpeg") || (mediaType == "video" && !CookHelper::isSupportedVideoFile("upload" + extension)))
	{
		message = "不支持的视频格式";
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	pruneExpiredNoLock(nowSeconds());
	if (m_sessions.size() >= kMaxUploadSessions)
	{
		message = "当前上传任务较多，请稍后重试";
		return false;
	}
	size_t userSessionCount = 0;
	for (std::map<std::string, Session>::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
	{
		if (it->second.m_userId == userId) ++userSessionCount;
	}
	if (userSessionCount >= kMaxSessionsPerUser)
	{
		message = "每个账号最多同时上传两个视频";
		return false;
	}
	const std::string id = makeUploadId(userId);
	if (id.empty())
	{
		message = "Unable to create secure upload identifier";
		return false;
	}
	const std::string root = uploadRoot();
	if (!CookHelper::ensureDirectoryTree(root))
	{
		message = "无法创建视频上传目录";
		return false;
	}
	if (!m_stagingScanned)
	{
		removeStalePartsNoLock(root);
		m_stagingScanned = true;
	}
	Session session;
	session.m_userId = userId;
	session.m_tempPath = CookHelper::appendPathSeparator(root) + id + ".part";
	session.m_relativeFilePath = g_config.m_videoStorageDirectoryName + "/" + id + extension;
	session.m_mediaType = mediaType;
	session.m_expectedBytes = expectedBytes;
	session.m_receivedBytes = 0;
	session.m_createdAtSeconds = nowSeconds();
	std::ofstream empty(session.m_tempPath.c_str(), std::ios::binary | std::ios::trunc);
	if (!empty.good())
	{
		message = "无法创建视频上传文件";
		return false;
	}
	empty.close();
	if (empty.fail())
	{
		std::remove(session.m_tempPath.c_str());
		message = "无法初始化视频上传文件";
		return false;
	}
	m_sessions[id] = session;
	upload.m_uploadId = id;
	upload.m_expectedBytes = expectedBytes;
	upload.m_receivedBytes = 0;
	upload.m_mediaType = mediaType;
	message = "ok";
	return true;
}

bool CookVideoUploadService::append(const std::string& userId,
                                    const std::string& uploadId,
                                    int64_t offset,
                                    const std::string& chunk,
                                    int64_t& receivedBytes,
                                    std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	pruneExpiredNoLock(nowSeconds());
	std::map<std::string, Session>::iterator it = m_sessions.find(uploadId);
	if (it == m_sessions.end() || it->second.m_userId != userId)
	{
		message = "上传任务不存在";
		return false;
	}
	Session& session = it->second;
	if (chunk.empty() || static_cast<int64_t>(chunk.size()) > kMaxChunkBytes || offset < 0 ||
	    offset > session.m_expectedBytes || static_cast<int64_t>(chunk.size()) > session.m_expectedBytes - offset)
	{
		message = "视频分块范围无效";
		return false;
	}
	if (offset < session.m_receivedBytes && offset + static_cast<int64_t>(chunk.size()) <= session.m_receivedBytes)
	{
		std::ifstream existing(session.m_tempPath.c_str(), std::ios::binary);
		std::string prior(chunk.size(), '\0');
		existing.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
		existing.read(&prior[0], static_cast<std::streamsize>(prior.size()));
		if (existing.gcount() == static_cast<std::streamsize>(prior.size()) && prior == chunk)
		{
			session.m_createdAtSeconds = nowSeconds();
			receivedBytes = session.m_receivedBytes;
			message = "ok";
			return true;
		}
		message = "重复上传分块内容不一致";
		return false;
	}
	if (offset != session.m_receivedBytes)
	{
		message = "视频分块偏移不连续";
		return false;
	}
	std::ofstream output(session.m_tempPath.c_str(), std::ios::binary | std::ios::app);
	if (!output.good())
	{
		message = "无法写入视频分块";
		return false;
	}
	output.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
	output.flush();
	if (!output.good())
	{
		output.close();
		std::remove(session.m_tempPath.c_str());
		m_sessions.erase(it);
		message = "写入视频分块失败";
		return false;
	}
	session.m_receivedBytes += static_cast<int64_t>(chunk.size());
	session.m_createdAtSeconds = nowSeconds();
	receivedBytes = session.m_receivedBytes;
	message = "ok";
	return true;
}

bool CookVideoUploadService::finish(const std::string& userId,
                                    const std::string& uploadId,
                                    std::string& relativeFilePath,
                                    int32_t& fileSizeBytes,
                                    std::string& mediaType,
                                    std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	pruneExpiredNoLock(nowSeconds());
	std::map<std::string, Session>::iterator it = m_sessions.find(uploadId);
	if (it == m_sessions.end() || it->second.m_userId != userId)
	{
		message = "上传任务不存在";
		return false;
	}
	Session session = it->second;
	if (session.m_receivedBytes != session.m_expectedBytes || session.m_expectedBytes > INT32_MAX)
	{
		message = "视频文件尚未完整上传";
		return false;
	}
	std::ifstream input(session.m_tempPath.c_str(), std::ios::binary | std::ios::ate);
	if (!input.good() || static_cast<std::streamoff>(input.tellg()) != static_cast<std::streamoff>(session.m_expectedBytes))
	{
		message = "已上传视频文件大小不匹配";
		return false;
	}
	input.close();
	const std::string finalPath = CookHelper::appendPathSeparator(CSystem::GetCurrentExePath()) + session.m_relativeFilePath;
	if (std::rename(session.m_tempPath.c_str(), finalPath.c_str()) != 0)
	{
		message = "保存已上传视频失败";
		return false;
	}
	m_sessions.erase(it);
	relativeFilePath = session.m_relativeFilePath;
	fileSizeBytes = static_cast<int32_t>(session.m_expectedBytes);
	mediaType = session.m_mediaType;
	message = "ok";
	return true;
}

void CookVideoUploadService::removeFile(const std::string& relativeFilePath)
{
	if (relativeFilePath.empty()) return;
	const std::string path = CookHelper::appendPathSeparator(CSystem::GetCurrentExePath()) + relativeFilePath;
	std::remove(path.c_str());
}

bool CookVideoUploadService::cancel(const std::string& userId, const std::string& uploadId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	std::map<std::string, Session>::iterator it = m_sessions.find(uploadId);
	if (it == m_sessions.end() || it->second.m_userId != userId) return false;
	std::remove(it->second.m_tempPath.c_str());
	m_sessions.erase(it);
	return true;
}