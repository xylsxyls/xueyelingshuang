#include "CookHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>

bool CookHelper::textToBool(const std::string& text, bool defaultValue)
{
	if (text == "1" || text == "true" || text == "TRUE")
	{
		return true;
	}
	if (text == "0" || text == "false" || text == "FALSE")
	{
		return false;
	}
	return defaultValue;
}

std::string CookHelper::boolToText(bool value)
{
	return value ? "1" : "0";
}

std::string CookHelper::joinVector(const std::vector<std::string>& values, char delimiter)
{
	std::string result;
	for (size_t i = 0; i < values.size(); ++i)
	{
		if (values[i].empty())
		{
			continue;
		}
		if (!result.empty())
		{
			result.push_back(delimiter);
		}
		result += values[i];
	}
	return result;
}

std::string CookHelper::taskShortId(const CookTask& task)
{
	if (!task.m_shortId.empty())
	{
		return task.m_shortId;
	}
	size_t dot = task.m_id.rfind('.');
	return dot == std::string::npos ? task.m_id : task.m_id.substr(dot + 1);
}

bool CookHelper::taskIdMatches(const CookTask& task, const std::string& id)
{
	if (id.empty())
	{
		return false;
	}
	return task.m_id == id || taskShortId(task) == id;
}

std::string CookHelper::shortDependencyId(const std::string& recipeId, const std::string& dependencyId)
{
	std::string prefix = recipeId + ".";
	if (dependencyId.find(prefix) == 0)
	{
		return dependencyId.substr(prefix.size());
	}
	return dependencyId;
}

std::string CookHelper::fullDependencyId(const std::string& recipeId, const std::string& dependencyId)
{
	if (dependencyId.empty())
	{
		return "";
	}
	std::string prefix = recipeId + ".";
	if (dependencyId.find(prefix) == 0)
	{
		return dependencyId;
	}
	return prefix + dependencyId;
}

std::string CookHelper::lowerAscii(const std::string& text)
{
	std::string result = text;
	for (size_t i = 0; i < result.size(); ++i)
	{
		result[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(result[i])));
	}
	return result;
}

std::string CookHelper::queryValue(const HttpRequest& request, const std::string& key)
{
	std::map<std::string, std::string>::const_iterator it = request.m_query.find(key);
	return it == request.m_query.end() ? "" : it->second;
}

int32_t CookHelper::queryInt(const HttpRequest& request, const std::string& key, int32_t defaultValue)
{
	std::string text = queryValue(request, key);
	if (text.empty())
	{
		return defaultValue;
	}
	return static_cast<int32_t>(CStringManager::atoi64(text.c_str()));
}

bool CookHelper::queryBool(const HttpRequest& request, const std::string& key, bool defaultValue)
{
	std::string text = queryValue(request, key);
	return text.empty() ? defaultValue : textToBool(text, defaultValue);
}

std::string CookHelper::base64Encode(const std::string& data)
{
	static const char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string result;
	result.reserve(((data.size() + 2) / 3) * 4);
	for (size_t index = 0; index < data.size(); index += 3)
	{
		uint32_t block = static_cast<unsigned char>(data[index]) << 16;
		size_t remain = data.size() - index;
		if (remain > 1)
		{
			block |= static_cast<unsigned char>(data[index + 1]) << 8;
		}
		if (remain > 2)
		{
			block |= static_cast<unsigned char>(data[index + 2]);
		}
		result.push_back(kAlphabet[(block >> 18) & 0x3F]);
		result.push_back(kAlphabet[(block >> 12) & 0x3F]);
		result.push_back(remain > 1 ? kAlphabet[(block >> 6) & 0x3F] : '=');
		result.push_back(remain > 2 ? kAlphabet[block & 0x3F] : '=');
	}
	return result;
}

char CookHelper::pathSeparator()
{
#ifdef _WIN32
	return '\\';
#else
	return '/';
#endif
}

std::string CookHelper::normalizePathSeparators(const std::string& path)
{
	std::string result = path;
	char separator = pathSeparator();
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] == '/' || result[i] == '\\')
		{
			result[i] = separator;
		}
	}
	return result;
}

std::string CookHelper::appendPathSeparator(const std::string& path)
{
	if (path.empty())
	{
		return path;
	}
	if (path[path.size() - 1] == '/' || path[path.size() - 1] == '\\')
	{
		return normalizePathSeparators(path);
	}
	return normalizePathSeparators(path) + pathSeparator();
}

bool CookHelper::ensureDirectoryTree(const std::string& directoryPath)
{
	std::string path = appendPathSeparator(directoryPath);
	if (path.empty())
	{
		return false;
	}
	if (CSystem::DirOrFileExist(path))
	{
		return true;
	}

	size_t start = 0;
#ifdef _WIN32
	if (path.size() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
	{
		start = 3;
	}
#endif
	while (start < path.size())
	{
		size_t end = path.find(pathSeparator(), start);
		if (end == std::string::npos)
		{
			end = path.size();
		}
		if (end > 0)
		{
			std::string current = path.substr(0, end);
			if (!current.empty() && !CSystem::DirOrFileExist(current))
			{
				if (!CSystem::CreateDir(current) && !CSystem::DirOrFileExist(current))
				{
					return false;
				}
			}
		}
		if (end == path.size())
		{
			break;
		}
		start = end + 1;
	}
	return CSystem::DirOrFileExist(path);
}

std::string CookHelper::quoteCommandArgument(const std::string& value)
{
	std::string result = "\"";
	for (size_t i = 0; i < value.size(); ++i)
	{
		if (value[i] == '"')
		{
			result += "\\\"";
		}
		else
		{
			result.push_back(value[i]);
		}
	}
	result += "\"";
	return result;
}

std::string CookHelper::shortenCommandOutput(const std::string& output)
{
	std::string result = output;
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] == '\r' || result[i] == '\n' || result[i] == '\t')
		{
			result[i] = ' ';
		}
	}
	size_t begin = 0;
	while (begin < result.size() && std::isspace(static_cast<unsigned char>(result[begin])))
	{
		++begin;
	}
	size_t end = result.size();
	while (end > begin && std::isspace(static_cast<unsigned char>(result[end - 1])))
	{
		--end;
	}
	result = result.substr(begin, end - begin);
	if (result.size() > 400)
	{
		result = result.substr(0, 400);
	}
	return result;
}

bool CookHelper::isJpegImageData(const std::string& data)
{
	return data.size() >= 4 &&
		static_cast<unsigned char>(data[0]) == 0xFF &&
		static_cast<unsigned char>(data[1]) == 0xD8 &&
		static_cast<unsigned char>(data[data.size() - 2]) == 0xFF &&
		static_cast<unsigned char>(data[data.size() - 1]) == 0xD9;
}

std::string CookHelper::hashHex128(const std::string& text)
{
	uint64_t first = 1469598103934665603ULL;
	uint64_t second = 1099511628211ULL;
	for (size_t i = 0; i < text.size(); ++i)
	{
		uint64_t value = static_cast<unsigned char>(text[i]);
		first ^= value;
		first *= 1099511628211ULL;
		second ^= value + 0x9e3779b97f4a7c15ULL + (second << 6) + (second >> 2);
		second *= 1469598103934665603ULL;
	}
	std::ostringstream oss;
	oss << std::hex << std::setfill('0') << std::setw(16) << first << std::setw(16) << second;
	return oss.str();
}

std::string CookHelper::supportedVideoSuffixList()
{
	return "mp4.m4v.mov.webm.avi.wmv.flv.mkv.3gp.mpg.mpeg.ts.m3u8.ogv";
}

bool CookHelper::isSupportedVideoFile(const std::string& path)
{
	size_t slash = path.find_last_of("/\\");
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
	{
		return false;
	}
	std::string suffix = lowerAscii(path.substr(dot + 1));
	return suffix == "mp4" ||
		suffix == "m4v" ||
		suffix == "mov" ||
		suffix == "webm" ||
		suffix == "avi" ||
		suffix == "wmv" ||
		suffix == "flv" ||
		suffix == "mkv" ||
		suffix == "3gp" ||
		suffix == "mpg" ||
		suffix == "mpeg" ||
		suffix == "ts" ||
		suffix == "m3u8" ||
		suffix == "ogv";
}

std::string CookHelper::normalizeVideoId(const std::string& videoId)
{
	return videoId.find("feed_") == 0 ? videoId.substr(5) : videoId;
}

std::string CookHelper::normalizeTargetKey(const std::string& targetKey)
{
	std::string prefix = "video:";
	if (targetKey.find(prefix) == 0)
	{
		return prefix + normalizeVideoId(targetKey.substr(prefix.size()));
	}
	return targetKey;
}

std::string CookHelper::videoContentType(const std::string& path)
{
	size_t slash = path.find_last_of("/\\");
	size_t dot = path.find_last_of('.');
	std::string suffix;
	if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
	{
		suffix = lowerAscii(path.substr(dot + 1));
	}
	if (suffix == "webm")
	{
		return "video/webm";
	}
	if (suffix == "mov")
	{
		return "video/quicktime";
	}
	if (suffix == "m4v")
	{
		return "video/x-m4v";
	}
	if (suffix == "avi")
	{
		return "video/x-msvideo";
	}
	if (suffix == "wmv")
	{
		return "video/x-ms-wmv";
	}
	if (suffix == "flv")
	{
		return "video/x-flv";
	}
	if (suffix == "mkv")
	{
		return "video/x-matroska";
	}
	if (suffix == "3gp")
	{
		return "video/3gpp";
	}
	if (suffix == "mpg" || suffix == "mpeg")
	{
		return "video/mpeg";
	}
	if (suffix == "ts")
	{
		return "video/mp2t";
	}
	if (suffix == "m3u8")
	{
		return "application/vnd.apple.mpegurl";
	}
	if (suffix == "ogv")
	{
		return "video/ogg";
	}
	return "video/mp4";
}

std::string CookHelper::imageContentType(const std::string& path)
{
	size_t slash = path.find_last_of("/\\");
	size_t dot = path.find_last_of('.');
	std::string suffix;
	if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
	{
		suffix = lowerAscii(path.substr(dot + 1));
	}
	if (suffix == "png")
	{
		return "image/png";
	}
	if (suffix == "webp")
	{
		return "image/webp";
	}
	if (suffix == "gif")
	{
		return "image/gif";
	}
	return "image/jpeg";
}
