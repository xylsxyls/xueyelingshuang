#pragma once
#include <stdint.h>
#include <string>
#include <vector>

/** 单个功能测试用例结果
*/
struct CookServerTestCaseResult
{
	/** 构造函数，初始化数值和布尔成员
	*/
	CookServerTestCaseResult();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 测试用例名称
	std::string m_name;
	// 测试失败或补充说明
	std::string m_message;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 测试是否通过
	bool m_pass;
	// 关键HTTP请求返回状态码，没有请求时为0
	int32_t m_statusCode;
	// 用例执行耗时，单位毫秒
	int32_t m_costMs;
};

/** 压力测试汇总结果
*/
struct CookServerStressResult
{
	/** 构造函数，初始化压力测试统计值
	*/
	CookServerStressResult();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 压力测试失败样例，最多记录前若干条
	std::vector<std::string> m_failureSamples;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	// 压力测试是否执行
	bool m_executed;
	// 压力测试是否全部通过
	bool m_pass;
	// 压力测试线程数量
	int32_t m_threadCount;
	// 每个线程循环次数
	int32_t m_loopCount;
	// HTTP请求总数
	int32_t m_totalRequests;
	// 成功请求数量
	int32_t m_successRequests;
	// 失败请求数量
	int32_t m_failedRequests;
	// 总耗时，单位毫秒
	int64_t m_totalCostMs;
	// 最大单请求耗时，单位毫秒
	int32_t m_maxCostMs;
	// 最小单请求耗时，单位毫秒
	int32_t m_minCostMs;
	// 压力测试墙钟耗时，单位毫秒
	int32_t m_wallCostMs;
};