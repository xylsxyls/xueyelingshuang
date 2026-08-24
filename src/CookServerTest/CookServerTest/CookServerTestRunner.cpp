#include "CookServerTestRunner.h"
#include "CookServerTestConfig.h"
#include "CookServerTestDocumentWriter.h"
#include "CookServerTestHelper.h"
#include "CookServerTestReport.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>
#include <ctime>
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
	const std::string phase1Account = userId("phase1_" + CStringManager::toStringInt32(static_cast<int32_t>(std::time(nullptr))));
	const std::string phase1Password = "cook1234";
	std::string phase1Token;
	std::string adToken;
	std::string feedVideoId;
	std::string customRecipeId = "custom_" + phase1Account;

	runCase("第一期注册接口", [this, &client, phase1Account, phase1Password, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/auth/register", authBody(phase1Account, phase1Password, "测试厨友"), response);
		statusCode = response.m_statusCode;
		phase1Token = responseJsonString(response, "token");
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

	runCase("第一期服务端语音播报", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		RapidJsonDocument body;
		body.setObject();
		body.addString("text", "开始淘米加水。预计用时五分钟。");
		body.addString("gender", "female");
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
			if (provider == "windows_sapi" && expectBodyContains(ttsResponse, "Windows SAPI synthesize failed", message))
			{
				message = "Windows SAPI unavailable on this machine: " + responseJsonString(ttsResponse, "message");
				return true;
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
		if (format == "wav" && (audioResponse.m_body.size() <= 44 || audioResponse.m_body.substr(0, 4) != "RIFF"))
		{
			message = "audio body is not wav";
			return false;
		}
		if (format == "mp3" && audioResponse.m_body.empty())
		{
			message = "audio body is empty mp3";
			return false;
		}
		return true;
	});

	runCase("第一期语音预生成队列", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		std::vector<std::string> texts;
		texts.push_back("开始淘米加水。预计用时5分钟。");
		texts.push_back("启动煮饭。预计用时10秒。");
		texts.push_back("并且。");
		texts.push_back("打散鸡蛋。预计用时2分钟。");
		texts.push_back("炒番茄回蛋。预计用时3分钟。");

		RapidJsonDocument body;
		body.setObject();
		body.addString("planId", "voice_prepare_test");
		body.addString("gender", "female");
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

	runCase("第一期未登录拦截", [this, &client](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.get("/api/recipes", response);
		statusCode = response.m_statusCode;
		return expectNetworkOk(response, message) &&
			expectStatus(response, 401, message) &&
			expectJsonOk(response, false, message);
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
		RapidJsonDocument finishBody;
		finishBody.setObject();
		finishBody.addString("adToken", adToken);
		finishBody.addInt("debugElapsedSeconds", 31);
		CookServerTestHttpResponse finishResponse;
		client.post("/api/wallet/ad/finish", finishBody.toString(), authHeaders(phase1Token), finishResponse);
		statusCode = finishResponse.m_statusCode;
		return !adToken.empty() &&
			expectNetworkOk(finishResponse, message) &&
			expectStatus(finishResponse, 200, message) &&
			expectJsonOk(finishResponse, true, message) &&
			expectBodyContains(finishResponse, "广告奖励领取成功", message);
	});

	runCase("第一期金币菜谱幂等加入", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		std::map<std::string, std::string> headers = authHeaders(phase1Token);
		headers["X-Idempotency-Key"] = "idem_coin_recipe";
		CookServerTestHttpResponse firstResponse;
		client.post("/api/recipes/join", joinRecipeBody("cook_000006", "idem_coin_recipe"), headers, firstResponse);
		if (!expectNetworkOk(firstResponse, message) || !expectStatus(firstResponse, 200, message) || !expectJsonOk(firstResponse, true, message))
		{
			statusCode = firstResponse.m_statusCode;
			return false;
		}
		CookServerTestHttpResponse secondResponse;
		client.post("/api/recipes/join", joinRecipeBody("cook_000006", "idem_coin_recipe"), headers, secondResponse);
		statusCode = secondResponse.m_statusCode;
		return expectNetworkOk(secondResponse, message) &&
			expectStatus(secondResponse, 200, message) &&
			expectJsonOk(secondResponse, true, message) &&
			expectBodyContains(secondResponse, "cook_000006", message);
	});

	runCase("第一期元宝菜谱加入", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse response;
		client.post("/api/recipes/join", joinRecipeBody("cook_000007", "idem_yuanbao_recipe"), authHeaders(phase1Token), response);
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
		syncRecipe.addValue("durationOverrides", syncOverrides);
		syncItems.pushValue(syncRecipe);
		syncBody.addValue("items", syncItems);
		CookServerTestHttpResponse syncResponse;
		client.post("/api/recipes/personalization/sync-times", syncBody.toString(), authHeaders(phase1Token), syncResponse);
		if (!expectNetworkOk(syncResponse, message) ||
			!expectStatus(syncResponse, 200, message) ||
			!expectJsonOk(syncResponse, true, message) ||
			!expectBodyContains(syncResponse, "\"updatedTaskCount\":1", message))
		{
			statusCode = syncResponse.m_statusCode;
			return false;
		}
		client.get("/api/recipes/personalization/detail?recipeId=cook_000001", authHeaders(phase1Token), detailResponse);
		if (!expectNetworkOk(detailResponse, message) ||
			!expectStatus(detailResponse, 200, message) ||
			!expectJsonOk(detailResponse, true, message) ||
			!expectBodyContains(detailResponse, "\"durationSeconds\":42", message))
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

		RapidJsonDocument followBody;
		followBody.setObject();
		followBody.addString("targetUserId", "creator_demo");
		CookServerTestHttpResponse followResponse;
		client.post("/api/follows/toggle", followBody.toString(), authHeaders(phase1Token), followResponse);
		statusCode = followResponse.m_statusCode;
		return expectNetworkOk(followResponse, message) &&
			expectStatus(followResponse, 200, message) &&
			expectJsonOk(followResponse, true, message) &&
			expectBodyContains(followResponse, "\"following\":true", message);
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

	runCase("第一期视频发布", [this, &client, &phase1Token](std::string& message, int32_t& statusCode) -> bool
	{
		CookServerTestHttpResponse uploadResponse;
		client.post("/api/videos/upload", "{\"mediaType\":\"video\",\"filePath\":\"test.mp4\",\"fileSizeBytes\":1024}", authHeaders(phase1Token), uploadResponse);
		if (!expectNetworkOk(uploadResponse, message) || !expectStatus(uploadResponse, 200, message) || !expectJsonOk(uploadResponse, true, message))
		{
			statusCode = uploadResponse.m_statusCode;
			return false;
		}
		RapidJsonDocument body;
		body.setObject();
		body.addString("videoId", "");
		body.addString("caption", "测试发布成果");
		CookServerTestHttpResponse emptyPublishResponse;
		client.post("/api/videos/publish", body.toString(), authHeaders(phase1Token), emptyPublishResponse);
		if (!expectNetworkOk(emptyPublishResponse, message) || !expectStatus(emptyPublishResponse, 200, message) || !expectJsonOk(emptyPublishResponse, false, message))
		{
			statusCode = emptyPublishResponse.m_statusCode;
			return false;
		}

		std::vector<std::string> recipeIds;
		recipeIds.push_back("cook_000006");
		body.addStringArray("recipeIds", recipeIds);
		CookServerTestHttpResponse publishResponse;
		client.post("/api/videos/publish", body.toString(), authHeaders(phase1Token), publishResponse);
		statusCode = publishResponse.m_statusCode;
		return expectNetworkOk(publishResponse, message) &&
			expectStatus(publishResponse, 200, message) &&
			expectJsonOk(publishResponse, true, message) &&
			expectBodyContains(publishResponse, "published", message);
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
			message = "feed video list is empty, check CookServer res/video deployment";
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
	recipe.addString("coverColor", "#4f8f5b");
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
	potato.addString("amount", "1个");
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
	return body.toString();
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
	return document.toString();
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
