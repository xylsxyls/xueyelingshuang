#pragma once
#include <string>

/** CookServerTest通用辅助函数，负责测试程序中不属于HTTP客户端或报告对象的通用逻辑
*/
class CookServerTestHelper
{
public:
	/** 获取只读视频素材路径，优先使用新目录，兼容历史res层
	@return 配置资源根下的视频文件路径，不创建或迁移素材
	*/
	static std::string videoFixturePath();

	/** 将当前进程本地代码页文本转换为UTF-8文本
	@param [in] text 当前进程本地代码页文本
	@return Windows下返回UTF-8文本，非Windows平台原样返回
	*/
	static std::string localTextToUtf8(const std::string& text);

	/** 将UTF-8文本转换为当前进程本地代码页文本
	@param [in] text UTF-8文本
	@return Windows下返回本地代码页文本，非Windows平台原样返回
	*/
	static std::string utf8ToLocalText(const std::string& text);
};