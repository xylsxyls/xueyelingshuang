#include "CookServerTestRunner.h"
#include "CookServerTestConfig.h"
#include "CookServerTestHelper.h"
#include "CookServerTestReport.h"
#include "CookServerTestStressTask.h"
#include "CookServerTestMediaCases.h"
#include "CookServerTestFeedCases.h"
#include "CookServerTestMenuSocialCases.h"
#include "CookServerTestCoverCases.h"
#include "CookServerTestUiRefineCases.h"
#include "CookServerTestIdentityCases.h"
#include "CookServerTestRecipeEditingCases.h"
#include "CookServerTestReviewCases.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>

CookServerTestRunner::CookServerTestRunner()
{

}

CookServerTestRunner::~CookServerTestRunner()
{

}

bool CookServerTestRunner::run()
{
	runFunctionalTests();
	runStressTests();

	bool allPass = true;
	for (size_t i = 0; i < m_caseResults.size(); ++i)
	{
		if (!m_caseResults[i].m_pass && !m_caseResults[i].m_notRun)
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
	result.m_id = static_cast<int32_t>(m_caseResults.size()) + 1;
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
	result.m_notRun = result.m_message.find("NOT-RUN: ") == 0;
	if (result.m_notRun) result.m_pass = false;
	result.m_costMs = CSystem::GetHighTickCountMilliRunTime(beginTime);
	m_caseResults.push_back(result);
	std::cout << (result.m_notRun ? "[NOT-RUN] " : result.m_pass ? "[PASS] " : "[FAIL] ") << result.m_id << " " << result.m_name << std::endl;
}

void CookServerTestRunner::runFunctionalTests()
{
	CookServerTestHttpClient client(g_cookServerTestConfig.m_host,
		g_cookServerTestConfig.m_port,
		g_cookServerTestConfig.m_timeoutMs,
		g_cookServerTestConfig.m_maxResponseBytes);
	const std::string phase1Account = userId("phase1_" + CStringManager::toStringInt32(static_cast<int32_t>(std::time(nullptr))));
	const std::string baseUserId = phase1Account;
	const std::string phase1Password = "cook1234";
	std::string phase1Token;
	std::string adToken;
	std::string feedVideoId;
	std::string createdVideoId;
	std::string customRecipeId = "custom_" + phase1Account;

	runCase("第一期注册接口", [this, &client, phase1Account, phase1Password, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/auth/register", authBody(phase1Account, phase1Password, "测试厨友"), response);
		statusCode = response.m_statusCode;
		phase1Token = responseJsonString(response, "token");
		m_authToken = phase1Token;
        RapidJsonDocument registered;
        registered.parse(response.m_body);
        m_authUserId = registered.getValue("account").getStringOrDefault("userId", "");
		client.setDefaultToken(phase1Token);
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"token\"", message) &&
			!phase1Token.empty();
	});

	runCase("第一期会话接口", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/auth/session", authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"account\"", message);
	});

	runCase("第一期服务端中文男女声播报与缓存", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		const char* genders[] = { "female", "male" };
		std::string previousKey;
		for (size_t genderIndex = 0; genderIndex < 2; ++genderIndex)
		{
			RapidJsonDocument body;
			body.setObject();
			body.addString("text", CStringManager::UnicodeToUtf8(L"开始淘米加水。预计用时五分钟。"));
			body.addString("gender", genders[genderIndex]);
			CookServerTestHttpResponse ttsResponse;
			client.post("/api/voice/tts", body.toString(), authHeaders(phase1Token), ttsResponse);
			statusCode = ttsResponse.m_statusCode;
			if (!expectNetworkOk(ttsResponse, message) ||
				!expectStatus(ttsResponse, 200, message) ||
				!expectBodyContains(ttsResponse, "\"provider\"", message) ||
				!expectBodyContains(ttsResponse, "\"format\"", message))
			{
				return false;
			}
			if (!responseJsonOk(ttsResponse))
			{
				std::string provider = responseJsonString(ttsResponse, "provider");
				if (provider == "disabled" && g_cookServerTestConfig.m_allowDisabledVoice &&
					expectBodyContains(ttsResponse, "server tts disabled", message))
				{
					message = "NOT-RUN: voice explicitly disabled; audio synthesis not tested";
					return false;
				}
				message = "tts response not ok: " + ttsResponse.m_body;
				return false;
			}
			if (!expectBodyContains(ttsResponse, "\"audioUrl\"", message) ||
				!expectBodyContains(ttsResponse, "\"audioKey\"", message))
			{
				return false;
			}

			std::string audioKey = responseJsonString(ttsResponse, "audioKey");
			std::string format = responseJsonString(ttsResponse, "format");
			if (audioKey.empty())
			{
				message = "audioKey is empty";
				return false;
			}
			CookServerTestHttpResponse audioResponse;
			client.get("/api/voice/audio?key=" + audioKey, audioResponse);
			statusCode = audioResponse.m_statusCode;
			if (!expectNetworkOk(audioResponse, message) || !expectStatus(audioResponse, 200, message))
			{
				return false;
			}
			if (format == "wav" && (audioResponse.m_body.size() <= 1000 || (audioResponse.m_body.substr(0, 4) != "RIFF" || audioResponse.m_body.substr(8, 4) != "WAVE")))
			{
				message = "audio body is not wav";
				return false;
			}
			if (format == "mp3" && audioResponse.m_body.empty())
			{
				message = "audio body is empty mp3";
				return false;
			}
			if (!previousKey.empty() && audioKey == previousKey)
			{
				message = "male and female audio keys must differ";
				return false;
			}
			previousKey = audioKey;
			const std::string badKeys[] = { audioKey.substr(1), "../" + audioKey,
				audioKey.substr(0, audioKey.size() - 4) + ".txt", audioKey.substr(0, audioKey.size() - 5) + "g.wav" };
			for (size_t badIndex = 0; badIndex < 4; ++badIndex)
			{
				CookServerTestHttpResponse badResponse;
				client.get("/api/voice/audio?key=" + CStringManager::UrlEncode(badKeys[badIndex]), badResponse);
				if (!expectNetworkOk(badResponse, message) || !expectStatus(badResponse, 404, message)) return false;
			}
			CookServerTestHttpResponse repeatResponse;
			client.post("/api/voice/tts", body.toString(), authHeaders(phase1Token), repeatResponse);
			if (!expectNetworkOk(repeatResponse, message) || !expectJsonOk(repeatResponse, true, message) ||
				responseJsonString(repeatResponse, "audioKey") != audioKey)
			{
				message = "repeat synthesis key must remain stable";
				return false;
			}
		}
		return true;
	});

	runCase("第一期语音预生成队列", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> texts;
		texts.push_back(CStringManager::UnicodeToUtf8(L"开始淘米加水。预计用时5分钟。"));
		texts.push_back(CStringManager::UnicodeToUtf8(L"启动煮饭。预计用时10秒。"));
		texts.push_back(CStringManager::UnicodeToUtf8(L"并且。"));
		texts.push_back(CStringManager::UnicodeToUtf8(L"打散鸡蛋。预计用时2分钟。"));
		texts.push_back(CStringManager::UnicodeToUtf8(L"炒番茄回蛋。预计用时3分钟。"));

		RapidJsonDocument body;
		body.setObject();
		body.addString("planId", "voice_prepare_test");
		body.addString("gender", "male");
		body.addInt("requiredCount", 4);
		body.addBool("includeRequiredAudio", true);
		body.addInt("requiredAudioWaitMilliseconds", 5000);
		body.addStringArray("texts", texts);
		CookServerTestHttpResponse prepareResponse;
		client.post("/api/voice/prepare", body.toString(), authHeaders(phase1Token), prepareResponse);
		statusCode = prepareResponse.m_statusCode;
		if (!expectNetworkOk(prepareResponse, message) ||
			!expectStatus(prepareResponse, 200, message) ||
			!expectJsonOk(prepareResponse, true, message) ||
			!expectBodyContains(prepareResponse, "\"requiredCount\":4", message) ||
			!expectBodyContains(prepareResponse, "\"requiredAudioIncluded\":true", message) ||
			!expectBodyContains(prepareResponse, "\"requiredAudioReadyCount\"", message) ||
			!expectBodyContains(prepareResponse, "\"clientConcurrency\"", message) ||
			!expectBodyContains(prepareResponse, "\"fetchWaitMilliseconds\"", message))
		{
			return false;
		}

		RapidJsonDocument prepareJson;
		if (!prepareJson.parse(prepareResponse.m_body) || !prepareJson.isObject())
		{
			message = "voice prepare response json parse failed";
			return false;
		}
		std::vector<RapidJsonValue> items = prepareJson.getArrayValueOrEmpty("items");
		if (items.size() != texts.size())
		{
			message = "voice prepare item count mismatch";
			return false;
		}
		if (items[0].getStringOrDefault("provider", "") == "windows_sapi" &&
			prepareJson.getIntOrDefault("requiredAudioReadyCount", 0) < 4)
		{
			message = "male required audio must be generated: " + prepareResponse.m_body;
			return false;
		}
		std::string voiceId = items[0].getStringOrDefault("voiceId", "");
		if (voiceId.empty())
		{
			message = "voiceId is empty";
			return false;
		}

		RapidJsonDocument fetchBody;
		fetchBody.setObject();
		fetchBody.addString("voiceId", voiceId);
		fetchBody.addBool("inlineAudio", false);
		fetchBody.addInt("waitMilliseconds", 0);
		CookServerTestHttpResponse fetchResponse;
		client.post("/api/voice/fetch", fetchBody.toString(), authHeaders(phase1Token), fetchResponse);
		statusCode = fetchResponse.m_statusCode;
		return expectNetworkOk(fetchResponse, message) &&
			expectStatus(fetchResponse, 200, message) &&
			expectBodyContains(fetchResponse, "\"voiceId\"", message) &&
			expectBodyContains(fetchResponse, "\"ready\"", message) &&
			expectBodyContains(fetchResponse, "\"status\"", message);
	});

	runCase("第一期无效token不能通过伪造userId访问", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.setDefaultToken("");
		client.get("/api/account?userId=forged_user", response);
		client.setDefaultToken(m_authToken);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 401, message) &&
			expectJsonOk(response, false, message) &&
			response.m_body.find("forged_user") == std::string::npos;
	});

	runCase("第一期签到奖励", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/wallet/checkin", "{}", authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"coins\"", message);
	});

	runCase("第一期广告奖励", [this, &client, &phase1Token, &adToken](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse startResponse;
	client.post("/api/wallet/ad/start", "{}", authHeaders(phase1Token), startResponse);
		if (!expectNetworkOk(startResponse, message) || !expectStatus(startResponse, 200, message) || !expectJsonOk(startResponse, true, message))
		{
			statusCode = startResponse.m_statusCode;
			return false;
		}
		adToken = responseJsonString(startResponse, "adToken");
		RapidJsonDocument startDocument;
		if (!startDocument.parse(startResponse.m_body) || !startDocument.isObject())
		{
			message = "ad start response JSON parse failed";
			return false;
		}
		int32_t requiredSeconds = startDocument.getIntOrDefault("requiredSeconds", -1);
		if (requiredSeconds < 0)
		{
			message = "requiredSeconds missing or negative";
			return false;
		}
		RapidJsonDocument finishBody;
		finishBody.setObject();
		finishBody.addString("adToken", adToken);
		CookServerTestHttpResponse finishResponse;
		client.post("/api/wallet/ad/finish", finishBody.toString(), authHeaders(phase1Token), finishResponse);
		statusCode = finishResponse.m_statusCode;
		if (adToken.empty() || !expectNetworkOk(finishResponse, message) || !expectStatus(finishResponse, 200, message))
		{
			return false;
		}
		if (requiredSeconds > 0)
		{
			if (!expectJsonOk(finishResponse, false, message) || !expectBodyContains(finishResponse, "广告观看时间不足", message))
			{
				return false;
			}
			std::this_thread::sleep_for(std::chrono::seconds(requiredSeconds + 1));
			client.post("/api/wallet/ad/finish", finishBody.toString(), authHeaders(phase1Token), finishResponse);
			statusCode = finishResponse.m_statusCode;
		}
		return expectNetworkOk(finishResponse, message) &&
			expectStatus(finishResponse, 200, message) &&
			expectJsonOk(finishResponse, true, message) &&
			expectBodyContains(finishResponse, "广告奖励领取成功", message);
	});

	runCase("第一期金币菜谱幂等购买", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		std::map<std::string, std::string> headers = authHeaders(phase1Token);
		headers["X-Idempotency-Key"] = "idem_coin_recipe";
		CookServerTestHttpResponse firstResponse;
		client.post("/api/recipes/purchase", joinRecipeBody("cook_000006", "idem_coin_recipe"), headers, firstResponse);
		if (!expectNetworkOk(firstResponse, message) || !expectStatus(firstResponse, 200, message) || !expectJsonOk(firstResponse, true, message))
		{
			statusCode = firstResponse.m_statusCode;
			return false;
		}
		CookServerTestHttpResponse secondResponse;
		client.post("/api/recipes/purchase", joinRecipeBody("cook_000006", "idem_coin_recipe"), headers, secondResponse);
		statusCode = secondResponse.m_statusCode;
		return expectNetworkOk(secondResponse, message) &&
			expectStatus(secondResponse, 200, message) &&
			expectJsonOk(secondResponse, true, message) &&
			expectBodyContains(secondResponse, "cook_000006", message);
	});

	runCase("第一期元宝菜谱购买", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/purchase", joinRecipeBody("cook_000007", "idem_yuanbao_recipe"), authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "cook_000007", message);
	});

	runCase("第一期个性化排程", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse saveResponse;
		client.post("/api/recipes/personalization/save", personalizationBody("cook_000001"), authHeaders(phase1Token), saveResponse);
		if (!expectNetworkOk(saveResponse, message) || !expectStatus(saveResponse, 200, message) || !expectJsonOk(saveResponse, true, message))
		{
			statusCode = saveResponse.m_statusCode;
			return false;
		}
		CookServerTestHttpResponse detailResponse;
		client.get("/api/recipes/personalization/detail?recipeId=cook_000001", authHeaders(phase1Token), detailResponse);
		if (!expectNetworkOk(detailResponse, message) ||
			!expectStatus(detailResponse, 200, message) ||
			!expectJsonOk(detailResponse, true, message) ||
			!expectBodyContains(detailResponse, "\"personalization\"", message) ||
			!expectBodyContains(detailResponse, "\"addedTasks\"", message) ||
			!expectBodyContains(detailResponse, "soak_meat", message))
		{
			statusCode = detailResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument syncBody;
		syncBody.setObject();
		RapidJsonValue syncItems;
		syncItems.setArray();
		RapidJsonValue syncRecipe;
		syncRecipe.setObject();
		syncRecipe.addString("recipeId", "cook_000001");
		RapidJsonValue syncOverrides;
		syncOverrides.setArray();
		RapidJsonValue syncOverride;
		syncOverride.setObject();
		syncOverride.addString("taskId", "soak_meat");
		syncOverride.addInt("durationSeconds", 42);
		syncOverrides.pushValue(syncOverride);
		RapidJsonValue addedOverride;
		addedOverride.setObject();
		addedOverride.addString("taskId", "cook_000001.test_plate");
		addedOverride.addInt("durationSeconds", 17);
		syncOverrides.pushValue(addedOverride);
		RapidJsonValue unknownOverride;
		unknownOverride.setObject();
		unknownOverride.addString("taskId", "not_a_recipe_step");
		unknownOverride.addInt("durationSeconds", 12345);
		syncOverrides.pushValue(unknownOverride);
		syncRecipe.addValue("durationOverrides", syncOverrides);
		syncItems.pushValue(syncRecipe);
		RapidJsonValue secondRecipe;
		secondRecipe.setObject();
		secondRecipe.addString("recipeId", "cook_000002");
		RapidJsonValue secondOverrides;
		secondOverrides.setArray();
		RapidJsonValue secondOverride;
		secondOverride.setObject();
		secondOverride.addString("taskId", "cook_000002.wash_rice");
		secondOverride.addInt("durationSeconds", 19);
		secondOverrides.pushValue(secondOverride);
		secondRecipe.addValue("durationOverrides", secondOverrides);
		syncItems.pushValue(secondRecipe);
		syncBody.addValue("items", syncItems);
		CookServerTestHttpResponse syncResponse;
		client.post("/api/recipes/personalization/sync-times", syncBody.toString(), authHeaders(phase1Token), syncResponse);
		if (!expectNetworkOk(syncResponse, message) ||
			!expectStatus(syncResponse, 200, message) ||
			!expectJsonOk(syncResponse, true, message) ||
			!expectBodyContains(syncResponse, "\"updatedTaskCount\":3", message) ||
			!expectBodyContains(syncResponse, "\"updatedRecipeCount\":2", message))
		{
			statusCode = syncResponse.m_statusCode;
			return false;
		}
		client.get("/api/recipes/personalization/detail?recipeId=cook_000001", authHeaders(phase1Token), detailResponse);
		if (!expectNetworkOk(detailResponse, message) ||
			!expectStatus(detailResponse, 200, message) ||
			!expectJsonOk(detailResponse, true, message) ||
			!expectBodyContains(detailResponse, "\"durationSeconds\":42", message) ||
			!expectBodyContains(detailResponse, "\"durationSeconds\":17", message) ||
			detailResponse.m_body.find("not_a_recipe_step") != std::string::npos)
		{
			statusCode = detailResponse.m_statusCode;
			return false;
		}
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		recipeIds.push_back("cook_000006");
		CookServerTestHttpResponse planResponse;
		client.post("/api/plan/preview", planBody("", recipeIds), authHeaders(phase1Token), planResponse);
		statusCode = planResponse.m_statusCode;
		RapidJsonDocument planDocument;
		if (!planDocument.parse(planResponse.m_body)) return false;
		const std::vector<RapidJsonValue> timeline = planDocument.getArrayValueOrEmpty("timeline");
		bool addedDurationApplied = false;
		bool secondDurationApplied = false;
		for (size_t i = 0; i < timeline.size(); ++i)
		{
			const std::string id = timeline[i].getStringOrDefault("id", "");
			if (id == "cook_000001.test_plate") addedDurationApplied = timeline[i].getIntOrDefault("durationSeconds", -1) == 17;
			if (id == "cook_000002.wash_rice") secondDurationApplied = timeline[i].getIntOrDefault("durationSeconds", -1) == 19;
		}
		if (!addedDurationApplied || !secondDurationApplied) { message = "synced actual durations must apply to the next plan"; return false; }
		return expectNetworkOk(planResponse, message) &&
			expectStatus(planResponse, 200, message) &&
			expectJsonOk(planResponse, true, message) &&
			expectBodyContains(planResponse, "已应用你的菜谱个性化设置", message) &&
			expectBodyContains(planResponse, "测试加步骤", message);
	});

	runCase("第一期评论接口", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		RapidJsonDocument body;
		body.setObject();
		body.addString("targetType", "recipe");
		body.addString("targetId", "cook_000006");
		body.addString("content", "测试评论，成品稳定。");
		body.addInt("score", 5);
		CookServerTestHttpResponse createResponse;
		client.post("/api/comments/create", body.toString(), authHeaders(phase1Token), createResponse);
		if (!expectNetworkOk(createResponse, message) || !expectStatus(createResponse, 200, message) || !expectJsonOk(createResponse, true, message))
		{
			statusCode = createResponse.m_statusCode;
			return false;
		}
		CookServerTestHttpResponse listResponse;
		client.get("/api/comments/list?targetType=recipe&targetId=cook_000006", authHeaders(phase1Token), listResponse);
		statusCode = listResponse.m_statusCode;
		return expectNetworkOk(listResponse, message) &&
			expectStatus(listResponse, 200, message) &&
			expectJsonOk(listResponse, true, message) &&
			expectBodyContains(listResponse, "测试评论", message);
	});

	runCase("第一期关注接口", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		RapidJsonDocument emptyBody;
		emptyBody.setObject();
		CookServerTestHttpResponse emptyResponse;
		client.post("/api/follows/toggle", emptyBody.toString(), authHeaders(phase1Token), emptyResponse);
		if (!expectNetworkOk(emptyResponse, message) || !expectStatus(emptyResponse, 200, message) || !expectJsonOk(emptyResponse, false, message))
		{
			statusCode = emptyResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument unknownBody;
		unknownBody.setObject();
		unknownBody.addString("targetUserId", "user_does_not_exist");
		CookServerTestHttpResponse unknownResponse;
		client.post("/api/follows/toggle", unknownBody.toString(), authHeaders(phase1Token), unknownResponse);
		if (!expectNetworkOk(unknownResponse, message) ||
			!expectStatus(unknownResponse, 200, message) ||
			!expectJsonOk(unknownResponse, false, message) ||
			!expectBodyContains(unknownResponse, "用户不存在", message))
		{
			statusCode = unknownResponse.m_statusCode;
			return false;
		}

		const std::string creatorAccount = userId("creator_demo");
		CookServerTestHttpResponse creatorRegisterResponse;
		client.post("/api/auth/register", authBody(creatorAccount, "cook1234", "测试作者"), creatorRegisterResponse);
		if (!expectNetworkOk(creatorRegisterResponse, message) ||
			!expectStatus(creatorRegisterResponse, 200, message) ||
			!expectJsonOk(creatorRegisterResponse, true, message))
		{
			statusCode = creatorRegisterResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument creatorRegisterDocument;
		if (!creatorRegisterDocument.parse(creatorRegisterResponse.m_body) || !creatorRegisterDocument.isObject())
		{
			message = "creator registration JSON parse failed";
			return false;
		}
		const std::string creatorUserId = creatorRegisterDocument.getValue("account").getStringOrDefault("userId", "");
		if (creatorUserId.empty())
		{
			message = "creator registration did not return a userId";
			return false;
		}

		RapidJsonDocument followBody;
		followBody.setObject();
		followBody.addString("targetUserId", creatorUserId);
		CookServerTestHttpResponse followResponse;
		client.post("/api/follows/toggle", followBody.toString(), authHeaders(phase1Token), followResponse);
		if (!expectNetworkOk(followResponse, message) ||
			!expectStatus(followResponse, 200, message) ||
			!expectJsonOk(followResponse, true, message) ||
			!expectBodyContains(followResponse, "\"following\":true", message))
		{
			statusCode = followResponse.m_statusCode;
			return false;
		}

		RapidJsonDocument systemFollowBody;
		systemFollowBody.setObject();
		systemFollowBody.addString("targetUserId", "qmzdc");
		CookServerTestHttpResponse systemFollowResponse;
		client.post("/api/follows/toggle", systemFollowBody.toString(), authHeaders(phase1Token), systemFollowResponse);
		if (!expectNetworkOk(systemFollowResponse, message) ||
			!expectStatus(systemFollowResponse, 200, message) ||
			!expectJsonOk(systemFollowResponse, true, message) ||
			!expectBodyContains(systemFollowResponse, "\"following\":true", message))
		{
			statusCode = systemFollowResponse.m_statusCode;
			return false;
		}

		CookServerTestHttpResponse accountResponse;
		client.get("/api/account", authHeaders(phase1Token), accountResponse);
		if (!expectNetworkOk(accountResponse, message) ||
			!expectStatus(accountResponse, 200, message) ||
			!expectJsonOk(accountResponse, true, message) ||
			!expectBodyContains(accountResponse, "\"followingUsers\"", message) ||
			!expectBodyContains(accountResponse, "\"friendUsers\"", message) ||
			!expectBodyContains(accountResponse, creatorUserId, message))
		{
			statusCode = accountResponse.m_statusCode;
			return false;
		}

		RapidJsonDocument messageBody;
		messageBody.setObject();
		messageBody.addString("targetUserId", creatorUserId);
		messageBody.addString("text", "测试私信");
		CookServerTestHttpResponse sendResponse;
		client.post("/api/messages/send", messageBody.toString(), authHeaders(phase1Token), sendResponse);
		if (!expectNetworkOk(sendResponse, message) ||
			!expectStatus(sendResponse, 200, message) ||
			!expectJsonOk(sendResponse, true, message) ||
			!expectBodyContains(sendResponse, "\"messageItem\"", message) ||
			!expectBodyContains(sendResponse, "\"type\":\"chat\"", message))
		{
			statusCode = sendResponse.m_statusCode;
			return false;
		}

		CookServerTestHttpResponse feedResponse;
		client.get("/api/feed?count=20", authHeaders(phase1Token), feedResponse);
		statusCode = feedResponse.m_statusCode;
		return expectNetworkOk(feedResponse, message) &&
			expectStatus(feedResponse, 200, message) &&
			expectJsonOk(feedResponse, true, message) &&
			expectBodyContains(feedResponse, "\"authorFollowing\":true", message);
	});

	runCase("第一期点赞接口", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		RapidJsonDocument body;
		body.setObject();
		body.addString("targetType", "recipe");
		body.addString("targetId", "cook_000006");
		CookServerTestHttpResponse response;
		client.post("/api/likes/toggle", body.toString(), authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"liked\":true", message);
	});

	runCase("第一期自定义菜谱草稿保存", [this, &client, &phase1Token, &customRecipeId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/custom/save", customRecipePublishBody(customRecipeId), authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		RapidJsonDocument saved;
        if (!saved.parse(response.m_body)) return false;
        customRecipeId = saved.getValue("recipe").getStringOrDefault("id", "");
        return !customRecipeId.empty() && expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, customRecipeId, message);
	});

	runCase("草稿菜谱仅作者可见且不可加入", [this, &client, &phase1Token, &customRecipeId](std::string& message, int32_t& statusCode) -> bool
	{
		RapidJsonDocument cycleRecipe;
		cycleRecipe.setObject();
		cycleRecipe.addString("schemaVersion", "1.0");
		cycleRecipe.addString("id", customRecipeId + "_cycle");
		cycleRecipe.addString("title", "Cycle test");
		cycleRecipe.addString("category", "test");
		RapidJsonValue cycleTasks;
		cycleTasks.setArray();
		RapidJsonValue cycleA;
		cycleA.setObject();
		cycleA.addString("id", "a");
		cycleA.addString("title", "A");
		cycleA.addInt("durationSeconds", 1);
		RapidJsonValue cycleADependencies;
		cycleADependencies.setArray();
		cycleADependencies.pushString("b");
		cycleA.addValue("dependencies", cycleADependencies);
		cycleTasks.pushValue(cycleA);
		RapidJsonValue cycleB;
		cycleB.setObject();
		cycleB.addString("id", "b");
		cycleB.addString("title", "B");
		cycleB.addInt("durationSeconds", 1);
		RapidJsonValue cycleBDependencies;
		cycleBDependencies.setArray();
		cycleBDependencies.pushString("a");
		cycleB.addValue("dependencies", cycleBDependencies);
		cycleTasks.pushValue(cycleB);
		cycleRecipe.addValue("tasks", cycleTasks);
		RapidJsonDocument cycleRequest;
		cycleRequest.setObject();
		cycleRequest.addString("recipeJson", cycleRecipe.toString());
		CookServerTestHttpResponse cycleResponse;
		client.post("/api/recipes/custom/save", cycleRequest.toString(), authHeaders(phase1Token), cycleResponse);
		if (!expectNetworkOk(cycleResponse, message) || !expectStatus(cycleResponse, 200, message) ||
		    !expectJsonOk(cycleResponse, false, message) ||
		    !expectBodyContains(cycleResponse, "task dependency cycle", message))
		{
			statusCode = cycleResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument safetyRecipe;
		safetyRecipe.setObject();
		safetyRecipe.addString("schemaVersion", "1.0");
		safetyRecipe.addString("id", customRecipeId + "_safety");
		safetyRecipe.addString("title", "Safety test");
		safetyRecipe.addString("category", "test");
		RapidJsonValue safetyTasks;
		safetyTasks.setArray();
		RapidJsonValue safetyTask;
		safetyTask.setObject();
		safetyTask.addString("id", "boil");
		safetyTask.addString("title", "Boil");
		safetyTask.addString("taskMode", "background");
		safetyTask.addInt("durationSeconds", 60);
		safetyTask.addString("safetyLevel", "danger");
		safetyTask.addString("backgroundWaitMode", "free");
		safetyTask.addBool("canLeaveKitchen", true);
		safetyTasks.pushValue(safetyTask);
		safetyRecipe.addValue("tasks", safetyTasks);
		RapidJsonDocument safetyRequest;
		safetyRequest.setObject();
		safetyRequest.addString("recipeJson", safetyRecipe.toString());
		CookServerTestHttpResponse safetyResponse;
		client.post("/api/recipes/custom/save", safetyRequest.toString(), authHeaders(phase1Token), safetyResponse);
		if (!expectNetworkOk(safetyResponse, message) || !expectStatus(safetyResponse, 200, message) ||
		    !expectJsonOk(safetyResponse, true, message) ||
		    !expectBodyContains(safetyResponse, "\"backgroundWaitMode\":\"watch\"", message) ||
		    !expectBodyContains(safetyResponse, "\"canLeaveKitchen\":false", message))
		{
			statusCode = safetyResponse.m_statusCode;
			return false;
		}

		CookServerTestHttpResponse saveResponse;
		client.post("/api/recipes/custom/save", customRecipePublishBody(customRecipeId), authHeaders(phase1Token), saveResponse);
		if (!expectNetworkOk(saveResponse, message) || !expectStatus(saveResponse, 200, message) || !expectJsonOk(saveResponse, true, message))
		{
			statusCode = saveResponse.m_statusCode;
			return false;
		}
		const std::string outsideAccount = userId("recipe_outside_" + CStringManager::toStringInt64(static_cast<int64_t>(CSystem::currentProcessPid())) + "_" + CStringManager::toStringInt64(static_cast<int64_t>(std::time(nullptr))));
		CookServerTestHttpResponse registerResponse;
		client.post("/api/auth/register", authBody(outsideAccount, "cook1234", "测试访客"), registerResponse);
		if (!expectNetworkOk(registerResponse, message) || !expectStatus(registerResponse, 200, message) || !expectJsonOk(registerResponse, true, message))
		{
			statusCode = registerResponse.m_statusCode;
			return false;
		}
		const std::string outsideToken = responseJsonString(registerResponse, "token");
		CookServerTestHttpResponse detailResponse;
		client.get("/api/recipes/detail?recipeId=" + CStringManager::UrlEncode(customRecipeId), authHeaders(outsideToken), detailResponse);
		if (!expectNetworkOk(detailResponse, message) || !expectStatus(detailResponse, 200, message) ||
		    !expectJsonOk(detailResponse, false, message) ||
		    detailResponse.m_body.find("测试青椒土豆丝") != std::string::npos)
		{
			statusCode = detailResponse.m_statusCode;
			if (message.empty()) message = "non-owner could read an unpublished recipe";
			return false;
		}
		RapidJsonDocument joinBody;
		joinBody.setObject();
		joinBody.addString("recipeId", customRecipeId);
		CookServerTestHttpResponse joinResponse;
		client.post("/api/recipes/join", joinBody.toString(), authHeaders(outsideToken), joinResponse);
		statusCode = joinResponse.m_statusCode;
		return expectNetworkOk(joinResponse, message) && expectStatus(joinResponse, 200, message) && expectJsonOk(joinResponse, false, message);
	});

	runCase("第一期自定义菜谱发布", [this, &client, &phase1Token, &customRecipeId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/custom/publish", customRecipePublishBody(customRecipeId), authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, customRecipeId, message);
	});

	runCase("分块上传视频并发布未挂载菜谱的作品", [this, &client, &phase1Token, &createdVideoId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse legacyResponse;
		client.post("/api/videos/upload", "{\"mediaType\":\"video\",\"filePath\":\"test.mp4\",\"fileSizeBytes\":1024}", authHeaders(phase1Token), legacyResponse);
		if (!expectNetworkOk(legacyResponse, message) || !expectStatus(legacyResponse, 404, message))
		{
			statusCode = legacyResponse.m_statusCode;
			return false;
		}
		CookServerTestHttpClient uploadClient(g_cookServerTestConfig.m_host,
			g_cookServerTestConfig.m_port,
			std::max(g_cookServerTestConfig.m_timeoutMs, 30000),
			std::max(g_cookServerTestConfig.m_maxResponseBytes, 50 * 1024 * 1024));

		const std::string samplePath = CookServerTestHelper::videoFixturePath();
		const std::string media = CSystem::readFile(samplePath);
		if (media.empty() || media.size() > 100U * 1024U * 1024U)
		{
			message = "video upload fixture is missing or exceeds 100 MiB: " + samplePath;
			return false;
		}
		RapidJsonDocument startBody;
		startBody.setObject();
		startBody.addString("mediaType", "video");
		startBody.addString("fileName", "cook_000001.mp4");
		startBody.addInt("fileSizeBytes", static_cast<int32_t>(media.size()));
		CookServerTestHttpResponse startResponse;
		uploadClient.post("/api/videos/upload/start", startBody.toString(), authHeaders(phase1Token), startResponse);
		if (!expectNetworkOk(startResponse, message) || !expectStatus(startResponse, 200, message) || !expectJsonOk(startResponse, true, message))
		{
			statusCode = startResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument startDocument;
		if (!startDocument.parse(startResponse.m_body) || !startDocument.isObject())
		{
			message = "video upload start JSON parse failed";
			return false;
		}
		const std::string uploadId = startDocument.getStringOrDefault("uploadId", "");
		const int32_t chunkBytes = startDocument.getIntOrDefault("chunkBytes", 0);
		if (uploadId.empty() || chunkBytes <= 0 || chunkBytes > 512 * 1024)
		{
			message = "video upload session or chunk size is invalid";
			return false;
		}
		std::map<std::string, std::string> chunkHeaders = authHeaders(phase1Token);
		chunkHeaders["Content-Type"] = "application/octet-stream";
		CookServerTestHttpResponse gapResponse;
		uploadClient.post("/api/videos/upload/chunk?uploadId=" + uploadId + "&offset=1", "x", chunkHeaders, gapResponse);
		if (!expectNetworkOk(gapResponse, message) || !expectStatus(gapResponse, 200, message) || !expectJsonOk(gapResponse, false, message))
		{
			statusCode = gapResponse.m_statusCode;
			return false;
		}
		for (size_t offset = 0; offset < media.size(); offset += static_cast<size_t>(chunkBytes))
		{
			const size_t length = std::min(static_cast<size_t>(chunkBytes), media.size() - offset);
			const std::string chunk(media.data() + offset, length);
			const std::string chunkPath = "/api/videos/upload/chunk?uploadId=" + uploadId + "&offset=" + CStringManager::toStringInt64(static_cast<int64_t>(offset));
			CookServerTestHttpResponse chunkResponse;
			uploadClient.post(chunkPath, chunk, chunkHeaders, chunkResponse);
			if (!expectNetworkOk(chunkResponse, message) || !expectStatus(chunkResponse, 200, message) || !expectJsonOk(chunkResponse, true, message))
			{
				statusCode = chunkResponse.m_statusCode;
				return false;
			}
			if (offset == 0)
			{
				CookServerTestHttpResponse duplicateResponse;
				uploadClient.post(chunkPath, chunk, chunkHeaders, duplicateResponse);
				if (!expectNetworkOk(duplicateResponse, message) || !expectStatus(duplicateResponse, 200, message) ||
				    !expectJsonOk(duplicateResponse, true, message) ||
				    !expectBodyContains(duplicateResponse, "\"receivedBytes\":" + CStringManager::toStringInt64(static_cast<int64_t>(length)), message))
				{
					statusCode = duplicateResponse.m_statusCode;
					return false;
				}
			}
		}
		RapidJsonDocument completeBody;
		completeBody.setObject();
		completeBody.addString("uploadId", uploadId);
		CookServerTestHttpResponse completeResponse;
		uploadClient.post("/api/videos/upload/complete", completeBody.toString(), authHeaders(phase1Token), completeResponse);
		if (!expectNetworkOk(completeResponse, message) || !expectStatus(completeResponse, 200, message) || !expectJsonOk(completeResponse, true, message))
		{
			statusCode = completeResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument completedDocument;
		if (!completedDocument.parse(completeResponse.m_body) || !completedDocument.isObject())
		{
			message = "video upload complete JSON parse failed";
			return false;
		}
		const RapidJsonValue uploadedVideo = completedDocument.getValue("video");
		const std::string videoId = uploadedVideo.getStringOrDefault("id", "");
		if (videoId.empty())
		{
			message = "completed upload did not return a video id";
			return false;
		}
		RapidJsonDocument publishBody;
		publishBody.setObject();
		publishBody.addString("videoId", videoId);
		publishBody.addString("caption", "chunk upload test");
		CookServerTestHttpResponse publishResponse;
		client.post("/api/videos/publish", publishBody.toString(), authHeaders(phase1Token), publishResponse);
		statusCode = publishResponse.m_statusCode;
		if (!expectNetworkOk(publishResponse, message) ||
		    !expectStatus(publishResponse, 200, message) ||
		    !expectJsonOk(publishResponse, true, message) ||
		    !expectBodyContains(publishResponse, "published", message))
		{
			return false;
		}
		CookServerTestHttpResponse segmentResponse;
		createdVideoId = videoId;
		uploadClient.get("/api/videos/segment?videoId=" + CStringManager::UrlEncode(videoId) + "&startSeconds=0&durationSeconds=1", segmentResponse);
		statusCode = segmentResponse.m_statusCode;
		if (!expectNetworkOk(segmentResponse, message) || !expectStatus(segmentResponse, 200, message)) return false;
		if (segmentResponse.m_body.find("ftyp") == std::string::npos ||
		    segmentResponse.m_headers.find("X-Video-Duration-Ms") == segmentResponse.m_headers.end())
		{
			message = "uploaded media did not produce a timed MP4 segment";
			return false;
		}
		return true;
	});

	runCase("第一期做菜成果记录", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		RapidJsonDocument body;
		body.setObject();
		body.addString("planId", "plan_test");
		body.addString("imagePath", "result.jpg");
		body.addStringArray("recipeIds", recipeIds);
		CookServerTestHttpResponse publishResponse;
		client.post("/api/cook/result/publish", body.toString(), authHeaders(phase1Token), publishResponse);
		if (!expectNetworkOk(publishResponse, message) || !expectStatus(publishResponse, 200, message) || !expectJsonOk(publishResponse, true, message))
		{
			statusCode = publishResponse.m_statusCode;
			return false;
		}
		CookServerTestHttpResponse historyResponse;
		client.get("/api/cook/history", authHeaders(phase1Token), historyResponse);
		statusCode = historyResponse.m_statusCode;
		return expectNetworkOk(historyResponse, message) &&
			expectStatus(historyResponse, 200, message) &&
			expectJsonOk(historyResponse, true, message) &&
			expectBodyContains(historyResponse, "plan_test", message);
	});

	runCase("健康检查接口", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/health", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"service\":\"CookServer\"", message) &&
			expectBodyContains(response, "\"port\":" + CStringManager::toStringInt32(g_cookServerTestConfig.m_port), message);
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

	runCase("菜单按分类分组菜谱列表", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/recipes?userId=" + baseUserId, response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 200, message) &&
			expectJsonOk(response, true, message) &&
			expectBodyContains(response, "\"categoryGroups\"", message) &&
			expectBodyContains(response, "\"name\"", message) &&
			expectBodyContains(response, "\"count\"", message) &&
			expectBodyContains(response, "\"account\"", message);
	});

	runCase("推荐流接口", [this, &client, &phase1Token, &feedVideoId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/feed?count=2", authHeaders(phase1Token), response);
		statusCode = response.m_statusCode;
		if (!expectNetworkOk(response, message) ||
			!expectStatus(response, 200, message) ||
			!expectJsonOk(response, true, message) ||
			!expectBodyContains(response, "\"tabs\"", message) ||
			!expectBodyContains(response, "\"feed\"", message) ||
			!expectBodyContains(response, "\"recipeIds\"", message) ||
			!expectBodyContains(response, "\"videoId\"", message) ||
			!expectBodyContains(response, "\"videoUrl\"", message) ||
			!expectBodyContains(response, "\"posterUrl\"", message) ||
			!expectBodyContains(response, "\"posterPrefetchCount\"", message))
		{
			return false;
		}
		RapidJsonDocument document;
		if (!document.parse(response.m_body) || !document.isObject())
		{
			message = "feed response json parse failed";
			return false;
		}
		if (document.getIntOrDefault("posterPrefetchCount", -1) < 0)
		{
			message = "posterPrefetchCount is invalid";
			return false;
		}
		std::vector<RapidJsonValue> feed = document.getArrayValueOrEmpty("feed");
		if (feed.empty())
		{
			message = "feed video list is empty, check CookServer external video resources";
			return false;
		}
		feedVideoId = feed[0].getStringOrDefault("videoId", "");
		if (feedVideoId.empty())
		{
			message = "feed videoId is empty";
			return false;
		}
		return true;
	});

	runCase("推荐视频Range与看过上报", [this, &client, &phase1Token, &feedVideoId](std::string& message, int32_t& statusCode) -> bool
	{
		if (feedVideoId.empty())
		{
			message = "feedVideoId is empty";
			statusCode = 0;
			return false;
		}
		std::map<std::string, std::string> rangeHeaders;
		rangeHeaders["Range"] = "bytes=0-1023";
		CookServerTestHttpResponse videoResponse;
		client.get("/api/videos/file?videoId=" + CStringManager::UrlEncode(feedVideoId), rangeHeaders, videoResponse);
		statusCode = videoResponse.m_statusCode;
		if (!expectNetworkOk(videoResponse, message) ||
			!expectStatus(videoResponse, 206, message) ||
			videoResponse.m_body.empty() ||
			videoResponse.m_headers.find("Content-Range") == videoResponse.m_headers.end())
		{
			if (message.empty())
			{
				message = "video range response invalid";
			}
			return false;
		}
		CookServerTestHttpResponse prefixedVideoResponse;
		client.get("/api/videos/file?videoId=" + CStringManager::UrlEncode("feed_" + feedVideoId), rangeHeaders, prefixedVideoResponse);
		statusCode = prefixedVideoResponse.m_statusCode;
		if (!expectNetworkOk(prefixedVideoResponse, message) ||
			!expectStatus(prefixedVideoResponse, 206, message) ||
			prefixedVideoResponse.m_body.empty() ||
			prefixedVideoResponse.m_headers.find("Content-Range") == prefixedVideoResponse.m_headers.end())
		{
			if (message.empty())
			{
				message = "prefixed video range response invalid";
			}
			return false;
		}

		CookServerTestHttpResponse posterResponse;
		client.get("/api/videos/poster?videoId=" + CStringManager::UrlEncode(feedVideoId), authHeaders(phase1Token), posterResponse);
		statusCode = posterResponse.m_statusCode;
		if (!expectNetworkOk(posterResponse, message))
		{
			return false;
		}
		if (posterResponse.m_statusCode == 200)
		{
			if (posterResponse.m_body.empty())
			{
				message = "video poster body is empty";
				return false;
			}
			if (posterResponse.m_headers.find("Content-Type") == posterResponse.m_headers.end() ||
				posterResponse.m_headers["Content-Type"].find("image/jpeg") == std::string::npos)
			{
				message = "video poster content type is not image/jpeg";
				return false;
			}
			if (posterResponse.m_body.size() < 4 ||
				static_cast<unsigned char>(posterResponse.m_body[0]) != 0xFF ||
				static_cast<unsigned char>(posterResponse.m_body[1]) != 0xD8 ||
				static_cast<unsigned char>(posterResponse.m_body[posterResponse.m_body.size() - 2]) != 0xFF ||
				static_cast<unsigned char>(posterResponse.m_body[posterResponse.m_body.size() - 1]) != 0xD9)
			{
				message = "video poster body is not jpeg";
				return false;
			}
		}
		else if (posterResponse.m_statusCode != 404)
		{
			message = "video poster response unexpected status " + CStringManager::toStringInt32(posterResponse.m_statusCode);
			return false;
		}

		RapidJsonDocument body;
		body.setObject();
		body.addString("videoId", "feed_" + feedVideoId);
		CookServerTestHttpResponse watchResponse;
		client.post("/api/feed/watch", body.toString(), authHeaders(phase1Token), watchResponse);
		statusCode = watchResponse.m_statusCode;
		return expectNetworkOk(watchResponse, message) &&
			expectStatus(watchResponse, 200, message) &&
			expectJsonOk(watchResponse, true, message) &&
			expectBodyContains(watchResponse, "\"watchedAtSeconds\"", message);
	});

	runCase("推荐视频时间片段与持久缓存", [this, &client, &feedVideoId](std::string& message, int32_t& statusCode) -> bool
	{
		if (feedVideoId.empty())
		{
			message = "feedVideoId is empty";
			statusCode = 0;
			return false;
		}
		const std::string path = "/api/videos/segment?videoId=" + CStringManager::UrlEncode(feedVideoId) + "&startSeconds=0&durationSeconds=1";
		CookServerTestHttpResponse firstResponse;
		client.get(path, firstResponse);
		statusCode = firstResponse.m_statusCode;
		if (!expectNetworkOk(firstResponse, message) || !expectStatus(firstResponse, 200, message) ||
		    firstResponse.m_body.size() < 12 || firstResponse.m_body.substr(4, 4) != "ftyp" ||
		    firstResponse.m_headers.find("X-Video-Segment-Start-Ms") == firstResponse.m_headers.end() ||
		    firstResponse.m_headers.find("X-Video-Duration-Ms") == firstResponse.m_headers.end())
		{
			if (message.empty()) message = "generated MP4 segment or timing headers are invalid";
			return false;
		}
		CookServerTestHttpResponse cachedResponse;
		client.get(path, cachedResponse);
		statusCode = cachedResponse.m_statusCode;
		if (!expectNetworkOk(cachedResponse, message) || !expectStatus(cachedResponse, 200, message) ||
		    cachedResponse.m_body != firstResponse.m_body)
		{
			if (message.empty()) message = "cached video segment differs from generated segment";
			return false;
		}
		CookServerTestHttpResponse invalidResponse;
		client.get("/api/videos/segment?videoId=" + CStringManager::UrlEncode(feedVideoId) + "&startSeconds=0&durationSeconds=61", invalidResponse);
		statusCode = invalidResponse.m_statusCode;
		if (!expectNetworkOk(invalidResponse, message) || !expectStatus(invalidResponse, 400, message)) return false;
		CookServerTestHttpResponse outsideResponse;
		client.get("/api/videos/segment?videoId=" + CStringManager::UrlEncode(feedVideoId) + "&startSeconds=2147483&durationSeconds=1", outsideResponse);
		statusCode = outsideResponse.m_statusCode;
		return expectNetworkOk(outsideResponse, message) && expectStatus(outsideResponse, 416, message);
	});

	runCase("推荐视频看过过滤与重置", [this, &client, &phase1Token, &feedVideoId](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse feedResponse;
		client.get("/api/feed?count=100", authHeaders(phase1Token), feedResponse);
		statusCode = feedResponse.m_statusCode;
		if (!expectNetworkOk(feedResponse, message) ||
			!expectStatus(feedResponse, 200, message) ||
			!expectJsonOk(feedResponse, true, message))
		{
			return false;
		}
		RapidJsonDocument feedDocument;
		if (!feedDocument.parse(feedResponse.m_body) || !feedDocument.isObject())
		{
			message = "feed response json parse failed";
			return false;
		}
		std::vector<RapidJsonValue> feed = feedDocument.getArrayValueOrEmpty("feed");
		if (feed.empty())
		{
			message = "feed video list is empty before watch reset test";
			return false;
		}

		std::vector<std::string> feedVideoIds;
		if (!feedVideoId.empty())
		{
			feedVideoIds.push_back(feedVideoId);
		}
		for (size_t i = 0; i < feed.size(); ++i)
		{
			std::string videoId = feed[i].getStringOrDefault("videoId", "");
			if (videoId.empty())
			{
				message = "feed videoId is empty in watch reset test";
				return false;
			}
			if (std::find(feedVideoIds.begin(), feedVideoIds.end(), videoId) == feedVideoIds.end())
			{
				feedVideoIds.push_back(videoId);
			}
		}
		if (feedVideoIds.empty())
		{
			message = "feed video ids is empty in watch reset test";
			return false;
		}

		auto buildExcludeIds = [](const std::vector<std::string>& videoIds) -> std::string
		{
			std::string excludeIdsText;
			for (size_t i = 0; i < videoIds.size(); ++i)
			{
				if (!excludeIdsText.empty())
				{
					excludeIdsText += ",";
				}
				excludeIdsText += videoIds[i];
			}
			return excludeIdsText;
		};

		std::string excludeIds = buildExcludeIds(feedVideoIds);
		std::string currentVideoId = feedVideoIds[feedVideoIds.size() - 1];
		CookServerTestHttpResponse loadedResetResponse;
		client.get("/api/feed?count=1&excludeIds=" + CStringManager::UrlEncode(excludeIds) + "&currentVideoId=" + CStringManager::UrlEncode(currentVideoId), authHeaders(phase1Token), loadedResetResponse);
		statusCode = loadedResetResponse.m_statusCode;
		if (!expectNetworkOk(loadedResetResponse, message) ||
			!expectStatus(loadedResetResponse, 200, message) ||
			!expectJsonOk(loadedResetResponse, true, message))
		{
			return false;
		}
		RapidJsonDocument loadedResetDocument;
		if (!loadedResetDocument.parse(loadedResetResponse.m_body) || !loadedResetDocument.isObject())
		{
			message = "feed loaded reset response json parse failed";
			return false;
		}
		if (!loadedResetDocument.getBoolOrDefault("resetSeen", false))
		{
			message = "feed resetSeen is false when all loaded videos are excluded";
			return false;
		}
		std::vector<RapidJsonValue> loadedResetFeed = loadedResetDocument.getArrayValueOrEmpty("feed");
		if (loadedResetFeed.empty())
		{
			message = "feed is empty when all loaded videos are excluded";
			return false;
		}
		if (feedVideoIds.size() > 1 && loadedResetFeed[0].getStringOrDefault("videoId", "") == currentVideoId)
		{
			message = "feed loaded reset returned current video immediately";
			return false;
		}

		for (size_t i = 0; i < feedVideoIds.size(); ++i)
		{
			RapidJsonDocument body;
			body.setObject();
			body.addString("videoId", feedVideoIds[i]);
			CookServerTestHttpResponse watchResponse;
			client.post("/api/feed/watch", body.toString(), authHeaders(phase1Token), watchResponse);
			statusCode = watchResponse.m_statusCode;
			if (!expectNetworkOk(watchResponse, message) ||
				!expectStatus(watchResponse, 200, message) ||
				!expectJsonOk(watchResponse, true, message))
			{
				return false;
			}
		}

		excludeIds = buildExcludeIds(feedVideoIds);
		currentVideoId = feedVideoIds[feedVideoIds.size() - 1];
		CookServerTestHttpResponse resetResponse;
		client.get("/api/feed?count=1&excludeIds=" + CStringManager::UrlEncode(excludeIds) + "&currentVideoId=" + CStringManager::UrlEncode(currentVideoId), authHeaders(phase1Token), resetResponse);
		statusCode = resetResponse.m_statusCode;
		if (!expectNetworkOk(resetResponse, message) ||
			!expectStatus(resetResponse, 200, message) ||
			!expectJsonOk(resetResponse, true, message))
		{
			return false;
		}
		RapidJsonDocument resetDocument;
		if (!resetDocument.parse(resetResponse.m_body) || !resetDocument.isObject())
		{
			message = "feed reset response json parse failed";
			return false;
		}
		if (!resetDocument.getBoolOrDefault("resetSeen", false))
		{
			message = "feed resetSeen is false after all videos watched";
			return false;
		}
		if (resetDocument.getArrayValueOrEmpty("feed").empty())
		{
			message = "feed is empty after watched list reset";
			return false;
		}
		std::vector<RapidJsonValue> resetFeed = resetDocument.getArrayValueOrEmpty("feed");
		if (feedVideoIds.size() > 1 && resetFeed[0].getStringOrDefault("videoId", "") == currentVideoId)
		{
			message = "feed reset returned current video immediately";
			return false;
		}
		return true;
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

	runCase("客户端不可直接充值金币", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/account/recharge", "{\"coins\":30}", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 404, message);
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
			expectBodyContains(response, "\"freeBlocks\"", message) &&
			expectBodyContains(response, "\"backgroundWaitMode\":\"watch\"", message);
	});

	runCase("菜单轻量排程预览", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		RapidJsonDocument body;
		body.setObject();
		body.addString("userId", baseUserId);
		body.addStringArray("recipeIds", recipeIds);
		body.addBool("summaryOnly", true);
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", body.toString(), response);
		statusCode = response.m_statusCode;
		if (!expectNetworkOk(response, message) ||
			!expectStatus(response, 200, message) ||
			!expectJsonOk(response, true, message) ||
			!expectBodyContains(response, "\"summaryOnly\":true", message) ||
			!expectBodyContains(response, "\"includeTimeBlocks\":false", message) ||
			!expectBodyContains(response, "\"summary\"", message))
		{
			return false;
		}
		if (response.m_body.find("\"timeline\"") != std::string::npos ||
			response.m_body.find("\"selectedRecipes\"") != std::string::npos ||
			response.m_body.find("\"tasks\"") != std::string::npos ||
			response.m_body.find("\"freeBlocks\"") != std::string::npos ||
			response.m_body.find("\"watchBlocks\"") != std::string::npos ||
			response.m_body.find("\"timeSegments\"") != std::string::npos)
		{
			message = "summaryOnly plan response contains detail fields";
			return false;
		}
		return true;
	});

	runCase("计划轻量排程预览", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000001");
		recipeIds.push_back("cook_000002");
		RapidJsonDocument body;
		body.setObject();
		body.addString("userId", baseUserId);
		body.addStringArray("recipeIds", recipeIds);
		body.addBool("summaryOnly", true);
		body.addBool("includeTimeBlocks", true);
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", body.toString(), response);
		statusCode = response.m_statusCode;
		if (!expectNetworkOk(response, message) ||
			!expectStatus(response, 200, message) ||
			!expectJsonOk(response, true, message) ||
			!expectBodyContains(response, "\"summaryOnly\":true", message) ||
			!expectBodyContains(response, "\"includeTimeBlocks\":true", message) ||
			!expectBodyContains(response, "\"summary\"", message) ||
			!expectBodyContains(response, "\"freeBlocks\"", message) ||
			!expectBodyContains(response, "\"watchBlocks\"", message) ||
			!expectBodyContains(response, "\"timeSegments\"", message) ||
			!expectBodyContains(response, "\"freeBlockMinimumSeconds\":120", message))
		{
			return false;
		}
		if (response.m_body.find("\"timeline\"") != std::string::npos ||
			response.m_body.find("\"selectedRecipes\"") != std::string::npos ||
			response.m_body.find("\"tasks\"") != std::string::npos)
		{
			message = "plan summary response contains full detail fields";
			return false;
		}
		return true;
	});

	runCase("米饭长等待优先排程", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000003");
		recipeIds.push_back("cook_000002");
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", planBody(baseUserId, recipeIds), response);
		statusCode = response.m_statusCode;
		if (!expectNetworkOk(response, message) ||
			!expectStatus(response, 200, message) ||
			!expectJsonOk(response, true, message))
		{
			return false;
		}

		RapidJsonDocument document;
		if (!document.parse(response.m_body) || !document.isObject())
		{
			message = "plan response json parse failed";
			return false;
		}

		RapidJsonValue summary = document.getValue("summary");
		int32_t totalSeconds = summary.getIntOrDefault("totalSeconds", 0);
		std::vector<RapidJsonValue> timeline = document.getArrayValueOrEmpty("timeline");
		int32_t washRiceStart = -1;
		int32_t cookRiceStart = -1;
		int32_t washTomatoStart = -1;
		int32_t scrambleEggsStart = -1;
		bool cookRiceBackground = false;
		bool cookRiceManualSkippable = true;
		std::string cookRiceWaitMode;
		for (size_t i = 0; i < timeline.size(); ++i)
		{
			std::string taskId = timeline[i].getStringOrDefault("id", "");
			int32_t startSeconds = timeline[i].getIntOrDefault("startSeconds", -1);
			if (taskId == "cook_000002.wash_rice")
			{
				washRiceStart = startSeconds;
			}
			else if (taskId == "cook_000002.cook_rice")
			{
				cookRiceStart = startSeconds;
				cookRiceBackground = timeline[i].getBoolOrDefault("background", false);
				cookRiceManualSkippable = timeline[i].getBoolOrDefault("manualSkippable", true);
				cookRiceWaitMode = timeline[i].getStringOrDefault("backgroundWaitMode", "");
			}
			else if (taskId == "cook_000003.wash_tomato")
			{
				washTomatoStart = startSeconds;
			}
			else if (taskId == "cook_000003.scramble_eggs")
			{
				scrambleEggsStart = startSeconds;
			}
		}

		if (washRiceStart != 0)
		{
			message = "wash_rice should start first";
			return false;
		}
		if (cookRiceStart < 0 || washTomatoStart < 0 || scrambleEggsStart < 0)
		{
			message = "required timeline task missing";
			return false;
		}
		if (cookRiceStart > washTomatoStart || cookRiceStart > scrambleEggsStart)
		{
			message = "cook_rice should start before tomato active cooking";
			return false;
		}
		if (!cookRiceBackground || cookRiceManualSkippable)
		{
			message = "cook_rice should be locked background timer";
			return false;
		}
		if (cookRiceWaitMode != "free")
		{
			message = "cook_rice background wait mode should be free";
			return false;
		}
		if (totalSeconds <= 0 || totalSeconds > 2600)
		{
			message = "tomato eggs with rice totalSeconds too long";
			return false;
		}
		return true;
	});

	runCase("未购买付费菜谱排程失败", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		const std::string otherAccount = userId("unowned");
		CookServerTestHttpResponse registerResponse;
		client.post("/api/auth/register", authBody(otherAccount, "cook1234", "未购买菜谱测试用户"), registerResponse);
		if (!expectNetworkOk(registerResponse, message) ||
			!expectStatus(registerResponse, 200, message) ||
			!expectJsonOk(registerResponse, true, message))
		{
			statusCode = registerResponse.m_statusCode;
			return false;
		}
		const std::string otherToken = responseJsonString(registerResponse, "token");
		if (otherToken.empty())
		{
			message = "unowned recipe test account has no auth token";
			return false;
		}
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000007");
		CookServerTestHttpResponse response;
		client.post("/api/plan/preview", planBody(otherAccount, recipeIds), authHeaders(otherToken), response);
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
			expectBodyContains(response, "\"planId\"", message) &&
			expectBodyContains(response, "\"clientConfig\"", message) &&
			expectBodyContains(response, "\"voiceInitialRequiredCount\"", message);
	});

	runCase("开始做菜完整排程", [this, &client, baseUserId](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000003");
		recipeIds.push_back("cook_000002");
		CookServerTestHttpResponse response;
		client.post("/api/plan/start", planBody(baseUserId, recipeIds), response);
		statusCode = response.m_statusCode;
		if (!expectNetworkOk(response, message) ||
			!expectStatus(response, 200, message) ||
			!expectJsonOk(response, true, message) ||
			!expectBodyContains(response, "\"account\"", message) ||
			!expectBodyContains(response, "\"selectedRecipes\"", message) ||
			!expectBodyContains(response, "\"timeline\"", message))
		{
			return false;
		}
		RapidJsonDocument document;
		if (!document.parse(response.m_body) || !document.isObject())
		{
			message = "full plan response json parse failed";
			return false;
		}
		std::vector<RapidJsonValue> selectedRecipes = document.getArrayValueOrEmpty("selectedRecipes");
		std::vector<RapidJsonValue> timeline = document.getArrayValueOrEmpty("timeline");
		if (selectedRecipes.size() != 2 || timeline.size() != 8)
		{
			std::ostringstream oss;
			oss << "full plan count mismatch selected=" << selectedRecipes.size() << " timeline=" << timeline.size();
			message = oss.str();
			return false;
		}
		return response.m_body.find("\"nextRequest\"") == std::string::npos;
	});

	runCase("非法JSON请求体按默认值处理", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/join", "{bad json", response);
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
		client.post("/api/recipes/join", body, response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 413, message) &&
		expectBodyContains(response, "Payload Too Large", message);
	});
	CookServerTestMediaCases::run(client, phase1Token, feedVideoId,
		[this](const std::string& name, const CookServerTestMediaCases::CaseFunction& action)
		{ runCase(name, action); });
	CookServerTestFeedCases::run(client, phase1Token, createdVideoId,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });
    CookServerTestMenuSocialCases::Run(client, createdVideoId,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });
    CookServerTestCoverCases::Run(client,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });
    CookServerTestUiRefineCases::Run(client,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });
    CookServerTestIdentityCases::run(client,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });
    CookServerTestRecipeEditingCases::run(client,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });
    CookServerTestReviewCases::run(client,
        [this](const std::string& name, const CookServerTestFeedCases::CaseFunction& action)
        { runCase(name, action); });


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
	std::vector<uint32_t> threads;
	std::vector<std::shared_ptr<CookServerTestStressTask>> tasks;
	bool launchOk = true;
	for (int32_t threadIndex = 0; threadIndex < g_cookServerTestConfig.m_stressThreadCount; ++threadIndex)
	{
		const uint32_t threadId = CTaskThreadManager::Instance().Init();
		std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
		if (!thread)
		{
			CTaskThreadManager::Instance().Uninit(threadId);
			launchOk = false;
			break;
		}
		std::shared_ptr<CookServerTestStressTask> task(new CookServerTestStressTask([this, threadIndex](const std::atomic<bool>& stop) -> void
		{
			CookServerTestHttpClient client(g_cookServerTestConfig.m_host,
				g_cookServerTestConfig.m_port,
				g_cookServerTestConfig.m_timeoutMs,
				g_cookServerTestConfig.m_maxResponseBytes);
			client.setDefaultToken(m_authToken);
			for (int32_t loopIndex = 0; loopIndex < g_cookServerTestConfig.m_stressLoopCount && !stop.load(); ++loopIndex)
			{
				int32_t costMs = 0;
				std::string error;
				bool ok = executeStressRequest(client, threadIndex, loopIndex, costMs, error);
				recordStressResult(ok, costMs, error);
			}
		}));
		thread->PostTask(task, 1);
		tasks.push_back(task);
		threads.push_back(threadId);
	}

	for (size_t i = 0; i < threads.size(); ++i)
	{
		CTaskThreadManager::Instance().WaitForEnd(threads[i]);
		if (tasks[i]->failed()) launchOk = false;
	}

	m_stressResult.m_wallCostMs = CSystem::GetHighTickCountMilliRunTime(beginTime);
	m_stressResult.m_pass = launchOk && m_stressResult.m_failedRequests == 0 &&
		m_stressResult.m_totalRequests == g_cookServerTestConfig.m_stressThreadCount * g_cookServerTestConfig.m_stressLoopCount;
	std::cout << (m_stressResult.m_pass ? "[PASS] " : "[FAIL] ") << "并发压力测试" << std::endl;
}

bool CookServerTestRunner::executeStressRequest(CookServerTestHttpClient& client, int32_t threadIndex, int32_t loopIndex, int32_t& costMs, std::string& error) const
{
	std::string currentUserId = userId("stress_" + CStringManager::toStringInt32(threadIndex));
	int32_t routeIndex = loopIndex % 13;
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
		requestOk = client.get(accountQueryPath(currentUserId), response);
		break;
	}
	case 7:
	{
		requestOk = client.post("/api/favorites/toggle", recipeBody(currentUserId, "cook_000002"), response);
		break;
	}
    case 8:
    {
        requestOk = client.get("/api/feed?mode=created&count=24", response);
        break;
    }
    case 9:
    {
        requestOk = client.get("/api/feed?mode=collection&count=24", response);
        break;
    }
    case 10:
    {
        requestOk = client.get("/api/users/profile?userId=" + m_authUserId, response);
        break;
    }
    case 11:
    {
        requestOk = client.get("/api/users/feed?mode=created&count=18&userId=" + m_authUserId, response);
        break;
    }
    case 12:
    {
        requestOk = client.get("/api/users/fans?count=30", response);
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
	if (m_stressResult.m_totalRequests == 1 || m_stressResult.m_minCostMs > costMs)
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

bool CookServerTestRunner::responseJsonOk(const CookServerTestHttpResponse& response) const
{
	RapidJsonDocument document;
	if (!document.parse(response.m_body) || !document.isObject())
	{
		return false;
	}
	return document.getBoolOrDefault("ok", false);
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

std::string CookServerTestRunner::responseJsonString(const CookServerTestHttpResponse& response, const char* key) const
{
	RapidJsonDocument document;
	if (!document.parse(response.m_body) || !document.isObject())
	{
		return "";
	}
	return document.getStringOrDefault(key, "");
}

std::map<std::string, std::string> CookServerTestRunner::authHeaders(const std::string& token) const
{
	std::map<std::string, std::string> headers;
	if (!token.empty())
	{
		headers["Authorization"] = "Bearer " + token;
	}
	return headers;
}

std::string CookServerTestRunner::authBody(const std::string& account, const std::string& password, const std::string& nickname) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("account", account);
	document.addString("password", password);
	document.addString("nickname", nickname);
	return document.toString();
}

std::string CookServerTestRunner::joinRecipeBody(const std::string& recipeId, const std::string& idempotencyKey) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("recipeId", recipeId);
	document.addString("idempotencyKey", idempotencyKey);
	return document.toString();
}

std::string CookServerTestRunner::customRecipePublishBody(const std::string& recipeId) const
{
	RapidJsonDocument recipe;
	recipe.setObject();
	recipe.addString("schemaVersion", "1.0");
	recipe.addString("id", recipeId);
	recipe.addString("title", "测试青椒土豆丝");
	recipe.addString("category", "蔬菜");
	recipe.addString("subtitle", "测试发布菜谱");
	recipe.addString("difficulty", "零基础");
	recipe.addString("author", "测试作者");
	recipe.addInt("priceCoins", 0);
	recipe.addBool("defaultOwned", false);
	recipe.addBool("systemRecipe", false);
	std::vector<std::string> tags;
	tags.push_back("测试");
	tags.push_back("快手");
	recipe.addStringArray("tags", tags);
	std::vector<std::string> tools;
	tools.push_back("炒锅");
	tools.push_back("砧板");
	recipe.addStringArray("tools", tools);

	RapidJsonValue ingredients;
	ingredients.setArray();
	RapidJsonValue potato;
	potato.setObject();
	potato.addString("name", "土豆");
	potato.addString("amount", "150克");
	ingredients.pushValue(potato);
	recipe.addValue("ingredients", ingredients);

	RapidJsonValue tasks;
	tasks.setArray();
	RapidJsonValue cutTask;
	cutTask.setObject();
	cutTask.addString("id", "cut");
	cutTask.addString("title", "切土豆丝");
	cutTask.addString("detail", "土豆切细丝，冲掉表面淀粉。");
	cutTask.addString("resource", "board");
	cutTask.addInt("durationSeconds", 180);
	cutTask.addBool("active", true);
	cutTask.addBool("canPause", true);
	cutTask.addBool("continuesDuringPause", false);
	cutTask.addBool("canLeaveKitchen", false);
	RapidJsonValue cutDeps;
	cutDeps.setArray();
	cutTask.addValue("dependencies", cutDeps);
	tasks.pushValue(cutTask);

	RapidJsonValue fryTask;
	fryTask.setObject();
	fryTask.addString("id", "fry");
	fryTask.addString("title", "大火快炒");
	fryTask.addString("detail", "热锅下油，大火翻炒到土豆丝断生。");
	fryTask.addString("resource", "stove");
	fryTask.addInt("durationSeconds", 240);
	fryTask.addBool("active", true);
	fryTask.addBool("canPause", false);
	fryTask.addBool("continuesDuringPause", false);
	fryTask.addBool("canLeaveKitchen", false);
	RapidJsonValue fryDeps;
	fryDeps.setArray();
	fryDeps.pushString("cut");
	fryTask.addValue("dependencies", fryDeps);
	tasks.pushValue(fryTask);
	recipe.addValue("tasks", tasks);

	RapidJsonDocument body;
	body.setObject();
	body.addString("recipeJson", recipe.toString());
	return CStringManager::AnsiToUtf8(body.toString());
}

std::string CookServerTestRunner::personalizationBody(const std::string& recipeId) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("recipeId", recipeId);
	RapidJsonValue disabled;
	disabled.setArray();
	document.addValue("disabledTaskIds", disabled);
	RapidJsonValue overrides;
	overrides.setArray();
	RapidJsonValue item;
	item.setObject();
	item.addString("taskId", "soak_meat");
	item.addInt("durationSeconds", 60);
	overrides.pushValue(item);
	document.addValue("durationOverrides", overrides);
	RapidJsonValue addedTasks;
	addedTasks.setArray();
	RapidJsonValue addedTask;
	addedTask.setObject();
	addedTask.addString("id", "test_plate");
	addedTask.addString("title", "测试加步骤");
	addedTask.addString("detail", "排程中验证新增步骤依赖收汁后执行。");
	addedTask.addString("resource", "board");
	addedTask.addInt("durationSeconds", 30);
	addedTask.addBool("active", true);
	addedTask.addBool("canPause", true);
	addedTask.addBool("continuesDuringPause", false);
	addedTask.addBool("canLeaveKitchen", false);
	RapidJsonValue dependencies;
	dependencies.setArray();
	dependencies.pushString("reduce_stir");
	addedTask.addValue("dependencies", dependencies);
	addedTasks.pushValue(addedTask);
	document.addValue("addedTasks", addedTasks);
	return CStringManager::AnsiToUtf8(document.toString());
}

std::string CookServerTestRunner::userId(const std::string& suffix) const
{
	return g_cookServerTestConfig.m_userIdPrefix + "_" + suffix;
}

std::string CookServerTestRunner::accountQueryPath(const std::string& userId) const
{
	return "/api/account?userId=" + userId;
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