#pragma once
#include "CookServerTestResult.h"
#include <string>
#include <vector>

/** CookServerTest报告生成器，负责把测试结果写入txt文件
*/
class CookServerTestReport
{
public:
	/** 构造函数，初始化空报告
	*/
	CookServerTestReport();

	/** 析构函数，报告对象不持有系统资源
	*/
	~CookServerTestReport();

	/** 生成完整报告文本
	@param [in] caseResults 功能测试结果
	@param [in] stressResult 压力测试结果
	@param [in] allPass 是否全部通过
	@return 返回报告文本
	*/
	std::string build(const std::vector<CookServerTestCaseResult>& caseResults, const CookServerStressResult& stressResult, bool allPass) const;

	/** 将报告写入文件
	@param [in] path 报告文件完整路径
	@param [in] text 报告文本
	@return 返回是否写入成功
	*/
	bool writeToFile(const std::string& path, const std::string& text) const;

private:
	/** 获取当前本地时间文本
	@return 返回yyyy-MM-dd HH:mm:ss格式时间
	*/
	std::string currentTimeText() const;

	/** 将当前进程本地代码页文本转换为UTF-8文本
	@param [in] text 本地代码页文本
	@return 返回UTF-8文本
	*/
	std::string localTextToUtf8(const std::string& text) const;
};