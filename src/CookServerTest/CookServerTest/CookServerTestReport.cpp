#include "CookServerTestReport.h"
#include "CookServerTestConfig.h"
#include "CookServerTestHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include <ctime>
#include <fstream>
#include <sstream>


CookServerTestReport::CookServerTestReport()
{

}

CookServerTestReport::~CookServerTestReport()
{

}

std::string CookServerTestReport::build(const std::vector<CookServerTestCaseResult>& caseResults, const CookServerStressResult& stressResult, bool allPass) const
{
	std::ostringstream oss;
	oss << "CookServerTest测试报告\r\n";
	oss << "生成时间：" << currentTimeText() << "\r\n";
	oss << "测试目标：http://" << g_cookServerTestConfig.m_host << ":" << g_cookServerTestConfig.m_port << "\r\n";
	oss << "整体结果：" << (allPass ? "通过" : "失败") << "\r\n";
	oss << "\r\n";

	oss << "一、功能测试结果\r\n";
	oss << "功能测试数量：" << caseResults.size() << "\r\n";
	for (size_t i = 0; i < caseResults.size(); ++i)
	{
		const CookServerTestCaseResult& result = caseResults[i];
		oss << (result.m_pass ? "[PASS] " : "[FAIL] ");
		oss << result.m_name;
		oss << " status=" << result.m_statusCode;
		oss << " costMs=" << result.m_costMs;
		if (!result.m_message.empty())
		{
			oss << " message=" << result.m_message;
		}
		oss << "\r\n";
	}
	oss << "\r\n";

	oss << "二、压力测试结果\r\n";
	if (!stressResult.m_executed)
	{
		oss << "压力测试未执行。\r\n";
	}
	else
	{
		int32_t averageCostMs = 0;
		if (stressResult.m_totalRequests > 0)
		{
			averageCostMs = static_cast<int32_t>(stressResult.m_totalCostMs / stressResult.m_totalRequests);
		}
		oss << "压力测试结果：" << (stressResult.m_pass ? "通过" : "失败") << "\r\n";
		oss << "线程数：" << stressResult.m_threadCount << "\r\n";
		oss << "每线程循环次数：" << stressResult.m_loopCount << "\r\n";
		oss << "总请求数：" << stressResult.m_totalRequests << "\r\n";
		oss << "成功请求数：" << stressResult.m_successRequests << "\r\n";
		oss << "失败请求数：" << stressResult.m_failedRequests << "\r\n";
		oss << "平均单请求耗时ms：" << averageCostMs << "\r\n";
		oss << "最大单请求耗时ms：" << stressResult.m_maxCostMs << "\r\n";
		oss << "最小单请求耗时ms：" << stressResult.m_minCostMs << "\r\n";
		oss << "压力测试总耗时ms：" << stressResult.m_wallCostMs << "\r\n";
		if (!stressResult.m_failureSamples.empty())
		{
			oss << "失败样例：\r\n";
			for (size_t i = 0; i < stressResult.m_failureSamples.size(); ++i)
			{
				oss << "- " << stressResult.m_failureSamples[i] << "\r\n";
			}
		}
	}
	oss << "\r\n";

	oss << "三、覆盖接口\r\n";
	oss << "GET /api/health\r\n";
	oss << "GET /api/recipes\r\n";
	oss << "GET /api/feed\r\n";
	oss << "GET /api/videos/file\r\n";
	oss << "GET /api/videos/poster\r\n";
	oss << "POST /api/feed/watch\r\n";
	oss << "GET /api/account\r\n";
	oss << "POST /api/account/recharge\r\n";
	oss << "POST /api/recipes/purchase\r\n";
	oss << "POST /api/favorites/toggle\r\n";
	oss << "POST /api/plan/preview\r\n";
	oss << "POST /api/plan/start\r\n";
	oss << "POST /api/voice/prepare\r\n";
	oss << "POST /api/voice/fetch\r\n";
	oss << "OPTIONS /api/health\r\n";
	oss << "GET /api/not_found\r\n";
	oss << "大请求体超过服务限制场景\r\n";
	return oss.str();
}

bool CookServerTestReport::writeToFile(const std::string& path, const std::string& text) const
{
	std::ofstream output(path.c_str(), std::ios::binary | std::ios::trunc);
	if (!output.is_open())
	{
		return false;
	}
	std::string utf8Text = CookServerTestHelper::localTextToUtf8(text);
	while (!utf8Text.empty() &&
		(utf8Text[utf8Text.size() - 1] == '\r' ||
		utf8Text[utf8Text.size() - 1] == '\n' ||
		utf8Text[utf8Text.size() - 1] == ' ' ||
		utf8Text[utf8Text.size() - 1] == '\t'))
	{
		utf8Text.erase(utf8Text.size() - 1);
	}
	output.write(utf8Text.data(), static_cast<std::streamsize>(utf8Text.size()));
	output.flush();
	return output.good();
}

std::string CookServerTestReport::currentTimeText() const
{
	std::time_t currentTime = std::time(nullptr);
	std::tm timeInfo;
#ifdef _WIN32
	localtime_s(&timeInfo, &currentTime);
#else
	localtime_r(&currentTime, &timeInfo);
#endif
	char buffer[20];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeInfo);
	return buffer;
}
