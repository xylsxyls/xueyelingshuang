#pragma once
#include "CookModels.h"
#include "HttpService/HttpServiceAPI.h"
#include <stdint.h>
#include <string>
#include <vector>

/** Cook业务辅助函数集合，只放Cook项目内复用的业务/协议辅助逻辑
*/
class CookHelper
{
public:
	/** 将持久化文本转换成布尔值
	@param [in] text 文本
	@param [in] defaultValue 无法识别时返回的默认值
	@return 返回布尔值
	*/
	static bool textToBool(const std::string& text, bool defaultValue);

	/** 将布尔值转换成持久化文本
	@param [in] value 布尔值
	@return 返回1或0
	*/
	static std::string boolToText(bool value);

	/** 拼接字符串数组，空字符串会被忽略
	@param [in] values 字符串数组
	@param [in] delimiter 分隔符
	@return 返回拼接文本
	*/
	static std::string joinVector(const std::vector<std::string>& values, char delimiter);

	/** 获取菜谱步骤短ID
	@param [in] task 菜谱步骤
	@return 返回短ID
	*/
	static std::string taskShortId(const CookTask& task);

	/** 判断步骤ID是否匹配完整ID或短ID
	@param [in] task 菜谱步骤
	@param [in] id 待匹配ID
	@return 返回是否匹配
	*/
	static bool taskIdMatches(const CookTask& task, const std::string& id);

	/** 将同菜谱内依赖ID转换成短ID
	@param [in] recipeId 菜谱ID
	@param [in] dependencyId 依赖ID
	@return 返回短ID或原始ID
	*/
	static std::string shortDependencyId(const std::string& recipeId, const std::string& dependencyId);

	/** 将依赖ID补全成完整ID
	@param [in] recipeId 菜谱ID
	@param [in] dependencyId 依赖ID
	@return 返回完整ID
	*/
	static std::string fullDependencyId(const std::string& recipeId, const std::string& dependencyId);

	/** ASCII小写转换
	@param [in] text 文本
	@return 返回小写文本
	*/
	static std::string lowerAscii(const std::string& text);

	/** 从HTTP查询参数读取字段
	@param [in] request HTTP请求
	@param [in] key 字段名
	@return 返回字段值，不存在时为空
	*/
	static std::string queryValue(const HttpRequest& request, const std::string& key);

	/** 从HTTP查询参数读取整数字段
	@param [in] request HTTP请求
	@param [in] key 字段名
	@param [in] defaultValue 不存在时的默认值
	@return 返回字段值或默认值
	*/
	static int32_t queryInt(const HttpRequest& request, const std::string& key, int32_t defaultValue);

	/** 从HTTP查询参数读取布尔字段
	@param [in] request HTTP请求
	@param [in] key 字段名
	@param [in] defaultValue 不存在时的默认值
	@return 返回字段值或默认值
	*/
	static bool queryBool(const HttpRequest& request, const std::string& key, bool defaultValue);

	/** Base64编码
	@param [in] data 二进制数据
	@return 返回Base64文本
	*/
	static std::string base64Encode(const std::string& data);

	/** 当前平台路径分隔符
	@return 返回路径分隔符
	*/
	static char pathSeparator();

	/** 统一路径分隔符
	@param [in] path 路径
	@return 返回统一后的路径
	*/
	static std::string normalizePathSeparators(const std::string& path);

	/** 给路径补尾部分隔符
	@param [in] path 路径
	@return 返回带尾部分隔符的路径
	*/
	static std::string appendPathSeparator(const std::string& path);

	/** 确保目录树存在
	@param [in] directoryPath 目录路径
	@return 返回是否存在或创建成功
	*/
	static bool ensureDirectoryTree(const std::string& directoryPath);

	/** 命令行参数加引号并转义双引号
	@param [in] value 参数值
	@return 返回安全参数
	*/
	static std::string quoteCommandArgument(const std::string& value);

	/** 将命令输出压缩成日志短文本
	@param [in] output 命令输出
	@return 返回短文本
	*/
	static std::string shortenCommandOutput(const std::string& output);

	/** 判断二进制内容是否是JPEG图片
	@param [in] data 二进制内容
	@return 返回是否像JPEG图片
	*/
	static bool isJpegImageData(const std::string& data);

	/** 生成128位十六进制哈希文本
	@param [in] text 输入文本
	@return 返回哈希文本
	*/
	static std::string hashHex128(const std::string& text);

	/** 首页视频支持的后缀列表，供CSystem::findFilePath使用
	@return 返回点号分隔的后缀文本
	*/
	static std::string supportedVideoSuffixList();

	/** 判断路径是否是当前允许播放的视频文件
	@param [in] path 文件路径
	@return 返回是否是支持的视频后缀
	*/
	static bool isSupportedVideoFile(const std::string& path);

	/** 规整视频ID，兼容早期推荐流临时ID前缀
	@param [in] videoId 原始视频ID
	@return 返回真实视频ID
	*/
	static std::string normalizeVideoId(const std::string& videoId);

	/** 规整点赞、收藏等目标key中的视频ID
	@param [in] targetKey 原始目标key
	@return 返回规整后的目标key
	*/
	static std::string normalizeTargetKey(const std::string& targetKey);

	/** 根据视频路径推断HTTP Content-Type
	@param [in] path 文件路径
	@return 返回视频MIME类型
	*/
	static std::string videoContentType(const std::string& path);

	/** 根据图片路径推断HTTP Content-Type
	@param [in] path 文件路径
	@return 返回图片MIME类型
	*/
	static std::string imageContentType(const std::string& path);
};
