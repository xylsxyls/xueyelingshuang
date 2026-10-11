#pragma once

#include <stdint.h>
#include <map>
#include <mutex>
#include <string>

struct VideoUploadInfo
{
	std::string m_uploadId;
	int64_t m_expectedBytes;
	int64_t m_receivedBytes;
	std::string m_mediaType;
};

/** Receives authenticated video uploads in bounded, ordered chunks. */
class CookVideoUploadService
{
public:
	CookVideoUploadService();

	bool start(const std::string& userId,
	           const std::string& mediaType,
	           const std::string& fileName,
	           int64_t expectedBytes,
	           int64_t maxBytes,
	           VideoUploadInfo& upload,
	           std::string& message);

	bool append(const std::string& userId,
	            const std::string& uploadId,
	            int64_t offset,
	            const std::string& chunk,
	            int64_t& receivedBytes,
	            std::string& message);

	bool finish(const std::string& userId,
	            const std::string& uploadId,
	            std::string& relativeFilePath,
	            int32_t& fileSizeBytes,
	            std::string& mediaType,
	            std::string& message);

	bool cancel(const std::string& userId, const std::string& uploadId);

	static void removeFile(const std::string& relativeFilePath);

private:
	struct Session
	{
		std::string m_userId;
		std::string m_tempPath;
		std::string m_relativeFilePath;
		std::string m_mediaType;
		int64_t m_expectedBytes;
		int64_t m_receivedBytes;
		int64_t m_createdAtSeconds;
	};

	void pruneExpiredNoLock(int64_t nowSeconds);
	void removeStalePartsNoLock(const std::string& root);
	std::map<std::string, Session> m_sessions;
	std::mutex m_mutex;
	bool m_stagingScanned;
};
