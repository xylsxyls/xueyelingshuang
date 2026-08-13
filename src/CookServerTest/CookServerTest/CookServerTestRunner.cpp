#include "CookServerTestRunner.h"
#include "CookServerTestConfig.h"
#include "CookServerTestDocumentWriter.h"
#include "CookServerTestHelper.h"
#include "CookServerTestReport.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <iostream>
#include <sstream>
#include <thread>

CookServerTestRunner::CookServerTestRunner()
{

}

CookServerTestRunner::~CookServerTestRunner()
{

}

bool CookServerTestRunner::run()
{
	CookServerTestDocumentWriter documentWriter;
	bool documentOk = documentWriter.writeDocuments();
	runCase("CookServer文档输出", [documentOk](std::string& message, int32_t& statusCode) -> bool
	{
		statusCode = 0;
		if (!documentOk)
		{
			message = "write CookServer documents failed";
			return false;
		}
		return true;
	});

	runFunctionalTests();
	runStressTests();

	bool allPass = true;
	for (size_t i = 0; i < m_caseResults.size(); ++i)
	{
		if (!m_caseResults[i].m_pass)
		{
			allPass = false;
			break;
		}
	}
	if (m_stressResult.m_executed && !m_stressResult.m_pass)
	{
		allPass = false;
	}

	bool reportOk = writeReport(allPass);
	std::cout << "CookServerTest report: " << g_cookServerTestConfig.reportPath() << std::endl;
	if (!reportOk)
	{
		std::cout << "CookServerTest write report failed" << std::endl;
		return false;
	}
	return allPass;
}

void CookServerTestRunner::runCase(const std::string& name, const std::function<bool(std::string&, int32_t&)>& testFunction)
{
	CookServerTestCaseResult result;
	result.m_name = name;
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	try
	{
		result.m_pass = testFunction(result.m_message, result.m_statusCode);
	}
	catch (...)
	{
		result.m_pass = false;
		result.m_message = "test case throw exception";
	}
	result.m_costMs = CSystem::GetHighTickCountMilliRunTime(beginTime);
	m_caseResults.push_back(result);
	std::cout << (result.m_pass ? "[PASS] " : "[FAIL] ") << result.m_name << std::endl;
}

void CookServerTestRunner::runFunctionalTests()
{
	CookServerTestHttpClient client(g_cookServerTestConfig.m_host,
		g_cookServerTestConfig.m_port,
		g_cookServerTestConfig.m_timeoutMs,
		g_cookServerTestConfig.m_maxResponseBytes);
	const std::string baseUserId = userId("functional");

	runCase("健康检查接口", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/health", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"service\":\"CookServer\"", message) &&
			expectBodyContains(response, "\"port\":5207", message);
	});

	runCase("菜谱列表接口", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/recipes?userId=" + baseUserId, response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"categories\"", message) &&
			expectBodyContains(response, "\"recipes\"", message) &&
			expectBodyContains(response, "\"ownedRecipeIds\"", message) &&
			expectBodyContains(response, "cook_000001", message) &&
			expectBodyContains(response, "cook_000006", message);
	});

	runCase("推荐流接口", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/feed?userId=" + baseUserId, response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"tabs\"", message) &&
			expectBodyContains(response, "\"feed\"", message) &&
			expectBodyContains(response, "\"recipeIds\"", message);
	});

	runCase("账号默认信息接口", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get(accountQueryPath(baseUserId), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, baseUserId, message) &&
			expectBodyContains(response, "\"coins\"", message);
	});

	runCase("OPTIONS预检接口", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.options("/api/health", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 204, message);
	});

	runCase("未知路由返回404", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/not_found", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 404, message);
	});

	runCase("非法充值金额", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/account/recharge", rechargeBody(baseUserId, -1), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, false, message) &&
			expectBodyContains(response, "coins must be", message);
	});

	runCase("正常充值", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/account/recharge", rechargeBody(baseUserId, 50), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "充值成功", message);
	});

	runCase("不存在菜谱购买失败", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/purchase", recipeBody(baseUserId, "not_exist_recipe"), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, false, message) &&
			expectBodyContains(response, "菜谱不存在", message);
	});

	runCase("付费菜谱购买", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/purchase", recipeBody(baseUserId, "cook_000006"), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "cook_000006", message);
	});

	runCase("收藏切换接口", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/favorites/toggle", recipeBody(baseUserId, "cook_000003"), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"favorite\"", message);
	});

	runCase("不存在菜谱收藏失败", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/favorites/toggle", recipeBody(baseUserId, "not_exist_recipe"), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, false, message) &&
			expectBodyContains(response, "菜谱不存在", message);
	});

	runCase("空排程请求失败", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", planBody(baseUserId, recipeIds), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, false, message) &&
			expectBodyContains(response, "recipeIds is required", message);
	});

	runCase("免费组合排程预览", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", planBody(baseUserId, recipeIds), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"summary\"", message) &&
			expectBodyContains(response, "\"timeline\"", message) &&
			expectBodyContains(response, "\"freeBlocks\"", message);
	});

	runCase("未购买付费菜谱排程失败", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		std::string otherUserId = userId("unowned");
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000007");
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", planBody(otherUserId, recipeIds), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, false, message) &&
			expectBodyContains(response, "请先购买菜谱", message);
	});

	runCase("开始做菜返回planId", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		CookServerTestHttpResponse response;
		client.post("/api/plan/start", planBody(baseUserId, recipeIds), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"planId\"", message);
	});

	runCase("非法JSON请求体按默认值处理", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/account/recharge", "{bad json", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, false, message);
	});

	runCase("超大请求体返回413", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		std::string body;
		body.assign(1024 * 1024 + 1, 'x');
		CookServerTestHttpResponse response;
		client.post("/api/account/recharge", body, response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 413, message) &&
			expectBodyContains(response, "Payload Too Large", message);
	});
}

void CookServerTestRunner::runStressTests()
{
	m_stressResult = CookServerStressResult();
	if (!g_cookServerTestConfig.m_enableStress)
	{
		return;
	}

	m_stressResult.m_executed = true;
	m_stressResult.m_threadCount = g_cookServerTestConfig.m_stressThreadCount;
	m_stressResult.m_loopCount = g_cookServerTestConfig.m_stressLoopCount;

	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	std::vector<std::thread> threads;
	threads.reserve(static_cast<size_t>(g_cookServerTestConfig.m_stressThreadCount));
	for (int32_t threadIndex = 0; threadIndex < g_cookServerTestConfig.m_stressThreadCount; ++threadIndex)
	{
		threads.push_back(std::thread([this, threadIndex]() -> void
		{
			CookServerTestHttpClient client(g_cookServerTestConfig.m_host,
				g_cookServerTestConfig.m_port,
				g_cookServerTestConfig.m_timeoutMs,
				g_cookServerTestConfig.m_maxResponseBytes);
			for (int32_t loopIndex = 0; loopIndex < g_cookServerTestConfig.m_stressLoopCount; ++loopIndex)
			{
				int32_t costMs = 0;
				std::string error;
				bool ok = executeStressRequest(client, threadIndex, loopIndex, costMs, error);
				recordStressResult(ok, costMs, error);
			}
		}));
	}

	for (size_t i = 0; i < threads.size(); ++i)
	{
		if (threads[i].joinable())
		{
			threads[i].join();
		}
	}

	m_stressResult.m_wallCostMs = CSystem::GetHighTickCountMilliRunTime(beginTime);
	m_stressResult.m_pass = m_stressResult.m_failedRequests == 0 && m_stressResult.m_totalRequests > 0;
	std::cout << (m_stressResult.m_pass ? "[PASS] " : "[FAIL] ") << "并发压力测试" << std::endl;
}

bool CookServerTestRunner::executeStressRequest(CookServerTestHttpClient& client, int32_t threadIndex, int32_t loopIndex, int32_t& costMs, std::string& error) const
{
	std::string currentUserId = userId("stress_" + CStringManager::toStringInt32(threadIndex));
	int32_t routeIndex = loopIndex % 8;
	CookServerTestHttpResponse response;
	bool requestOk = false;
	switch (routeIndex)
	{
	case 0:
	{
		requestOk = client.get("/api/health", response);
		break;
	}
	case 1:
	{
		requestOk = client.get("/api/recipes?userId=" + currentUserId, response);
		break;
	}
	case 2:
	{
		requestOk = client.get("/api/feed?userId=" + currentUserId, response);
		break;
	}
	case 3:
	{
		requestOk = client.get(accountQueryPath(currentUserId), response);
		break;
	}
	case 4:
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		requestOk = client.post("/api/plan/preview", planBody(currentUserId, recipeIds), response);
		break;
	}
	case 5:
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000003");
		recipeIds.push_back("cook_000004");
		requestOk = client.post("/api/plan/start", planBody(currentUserId, recipeIds), response);
		break;
	}
	case 6:
	{
		requestOk = client.post("/api/account/recharge", rechargeBody(currentUserId, 1), response);
		break;
	}
	case 7:
	{
		requestOk = client.post("/api/favorites/toggle", recipeBody(currentUserId, "cook_000002"), response);
		break;
	}
	default:
	{
		requestOk = false;
		break;
	}
	}

	costMs = response.m_costMs;
	if (!requestOk)
	{
		error = "network failed routeIndex=" + CStringManager::toStringInt32(routeIndex) + " error=" + response.m_error;
		return false;
	}
	if (response.m_statusCode != 200)
	{
		error = "unexpected status routeIndex=" + CStringManager::toStringInt32(routeIndex) + " status=" + CStringManager::toStringInt32(response.m_statusCode);
		return false;
	}
	std::string message;
	if (!expectJsonOk(response, true, message))
	{
		error = "json ok failed routeIndex=" + CStringManager::toStringInt32(routeIndex) + " message=" + message;
		return false;
	}
	return true;
}

void CookServerTestRunner::recordStressResult(bool ok, int32_t costMs, const std::string& message)
{
	std::lock_guard<std::mutex> lock(m_stressMutex);
	++m_stressResult.m_totalRequests;
	if (ok)
	{
		++m_stressResult.m_successRequests;
	}
	else
	{
		++m_stressResult.m_failedRequests;
		if (m_stressResult.m_failureSamples.size() < 20)
		{
			m_stressResult.m_failureSamples.push_back(message);
		}
	}
	m_stressResult.m_totalCostMs += costMs;
	if (m_stressResult.m_maxCostMs < costMs)
	{
		m_stressResult.m_maxCostMs = costMs;
	}
	if (m_stressResult.m_minCostMs == 0 || m_stressResult.m_minCostMs > costMs)
	{
		m_stressResult.m_minCostMs = costMs;
	}
}

bool CookServerTestRunner::expectNetworkOk(const CookServerTestHttpResponse& response, std::string& message) const
{
	if (!response.m_networkOk)
	{
		message = response.m_error.empty() ? "network failed" : response.m_error;
		return false;
	}
	return true;
}

bool CookServerTestRunner::expectStatus(const CookServerTestHttpResponse& response, int32_t expectedStatus, std::string& message) const
{
	if (response.m_statusCode != expectedStatus)
	{
		message = "status expected=" + CStringManager::toStringInt32(expectedStatus) +
			" actual=" + CStringManager::toStringInt32(response.m_statusCode) +
			" body=" + CookServerTestHelper::utf8ToLocalText(response.m_body);
		return false;
	}
	return true;
}

bool CookServerTestRunner::expectJsonOk(const CookServerTestHttpResponse& response, bool expectedOk, std::string& message) const
{
	RapidJsonDocument document;
	if (!document.parse(response.m_body) || !document.isObject())
	{
		message = "response body is not json object body=" + CookServerTestHelper::utf8ToLocalText(response.m_body);
		return false;
	}
	bool actualOk = document.getBoolOrDefault("ok", !expectedOk);
	if (actualOk != expectedOk)
	{
		message = "json ok expected=" + std::string(expectedOk ? "true" : "false") +
			" actual=" + std::string(actualOk ? "true" : "false") +
			" body=" + CookServerTestHelper::utf8ToLocalText(response.m_body);
		return false;
	}
	return true;
}

bool CookServerTestRunner::expectBodyContains(const CookServerTestHttpResponse& response, const std::string& text, std::string& message) const
{
	std::string utf8Text = CookServerTestHelper::localTextToUtf8(text);
	if (response.m_body.find(utf8Text) == std::string::npos)
	{
		message = "body not contains text=" + text + " body=" + CookServerTestHelper::utf8ToLocalText(response.m_body);
		return false;
	}
	return true;
}

std::string CookServerTestRunner::userId(const std::string& suffix) const
{
	return g_cookServerTestConfig.m_userIdPrefix + "_" + suffix;
}

std::string CookServerTestRunner::accountQueryPath(const std::string& userId) const
{
	return "/api/account?userId=" + userId;
}

std::string CookServerTestRunner::rechargeBody(const std::string& userId, int32_t coins) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("userId", userId);
	document.addInt("coins", coins);
	return document.toString();
}

std::string CookServerTestRunner::recipeBody(const std::string& userId, const std::string& recipeId) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("userId", userId);
	document.addString("recipeId", recipeId);
	return document.toString();
}

std::string CookServerTestRunner::planBody(const std::string& userId, const std::vector<std::string>& recipeIds) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("userId", userId);
	document.addStringArray("recipeIds", recipeIds);
	return document.toString();
}

bool CookServerTestRunner::writeReport(bool allPass) const
{
	CookServerTestReport report;
	std::string text = report.build(m_caseResults, m_stressResult, allPass);
	return report.writeToFile(g_cookServerTestConfig.reportPath(), text);
}