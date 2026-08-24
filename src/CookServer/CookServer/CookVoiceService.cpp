#include "CookVoiceService.h"
#include "Config.h"
#include "CookHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <sapi.h>
#endif

	const char kVoiceProviderWindowsSapi[] = "windows_sapi";
	const char kVoiceProviderCommand[] = "command";
	const char kVoiceProviderCommandWav[] = "command_wav";
	const char kVoiceProviderCommandMp3[] = "command_mp3";
	const char kVoiceProviderDisabled[] = "disabled";
	const char kVoiceFormatWav[] = "wav";
	const char kVoiceFormatMp3[] = "mp3";
	const char kVoiceContentTypeWav[] = "audio/wav";
	const char kVoiceContentTypeMp3[] = "audio/mpeg";
	const char kVoiceStatusPending[] = "pending";
	const char kVoiceStatusRunning[] = "running";
	const char kVoiceStatusReady[] = "ready";
	const char kVoiceStatusFailed[] = "failed";
	const int32_t kVoiceRequiredPriority = 100;
	const int32_t kVoiceNormalPriority = 10;
	const int32_t kVoiceTaskId = 5207001;

#ifdef _WIN32
	template <typename InterfaceType>
	void releaseCom(InterfaceType*& value)
	{
		if (value != nullptr)
		{
			value->Release();
			value = nullptr;
		}
	}

	std::string hresultText(HRESULT hr)
	{
		std::ostringstream oss;
		oss << "HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
		return oss.str();
	}

	std::wstring lowerWide(const std::wstring& text)
	{
		std::wstring result = text;
		for (size_t i = 0; i < result.size(); ++i)
		{
			if (result[i] >= L'A' && result[i] <= L'Z')
			{
				result[i] = static_cast<wchar_t>(result[i] - L'A' + L'a');
			}
		}
		return result;
	}

	bool tokenMatchesGender(ISpObjectToken* token, const std::string& gender)
	{
		if (token == nullptr)
		{
			return false;
		}
		ISpDataKey* attributes = nullptr;
		if (FAILED(token->OpenKey(L"Attributes", &attributes)) || attributes == nullptr)
		{
			return false;
		}

		LPWSTR genderValue = nullptr;
		bool matched = false;
		if (SUCCEEDED(attributes->GetStringValue(L"Gender", &genderValue)) && genderValue != nullptr)
		{
			std::wstring lower = lowerWide(genderValue);
			std::wstring expected = gender == "male" ? L"male" : L"female";
			matched = lower.find(expected) != std::wstring::npos;
		}
		if (genderValue != nullptr)
		{
			::CoTaskMemFree(genderValue);
		}
		releaseCom(attributes);
		return matched;
	}

	void trySelectVoiceByGender(ISpVoice* voice, const std::string& gender)
	{
		if (voice == nullptr)
		{
			return;
		}

		ISpObjectTokenCategory* category = nullptr;
		IEnumSpObjectTokens* enumTokens = nullptr;
		HRESULT hr = ::CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL, IID_ISpObjectTokenCategory, reinterpret_cast<void**>(&category));
		if (FAILED(hr) || category == nullptr)
		{
			return;
		}
		hr = category->SetId(SPCAT_VOICES, FALSE);
		if (SUCCEEDED(hr))
		{
			hr = category->EnumTokens(nullptr, nullptr, &enumTokens);
		}
		if (SUCCEEDED(hr) && enumTokens != nullptr)
		{
			ISpObjectToken* token = nullptr;
			ULONG fetched = 0;
			while (enumTokens->Next(1, &token, &fetched) == S_OK && fetched == 1)
			{
				if (tokenMatchesGender(token, gender))
				{
					voice->SetVoice(token);
					releaseCom(token);
					break;
				}
				releaseCom(token);
			}
		}
		releaseCom(enumTokens);
		releaseCom(category);
	}
#endif

	struct CookVoicePreparedJob
	{
		CookVoicePreparedJob() :
		m_required(false),
		m_order(0),
		m_queued(false),
		m_priority(0)
		{
			m_status = kVoiceStatusPending;
		}

		std::string m_voiceId;
		std::string m_text;
		std::string m_gender;
		std::string m_status;
		std::string m_message;
		bool m_required;
		int32_t m_order;
		bool m_queued;
		int32_t m_priority;
		CookVoiceTtsResult m_result;
		std::condition_variable m_condition;
	};

	struct CookVoicePreparedPlan
	{
		CookVoicePreparedPlan() :
		m_backgroundQueued(false)
		{
		}

		std::string m_planId;
		std::vector<std::string> m_requiredVoiceIds;
		std::vector<std::string> m_backgroundVoiceIds;
		bool m_backgroundQueued;
	};

	class CookVoiceGenerateTask : public CTask
	{
	public:
		CookVoiceGenerateTask(const std::string& voiceId) :
		CTask(kVoiceTaskId),
		m_voiceId(voiceId),
		m_cancelToken(new std::atomic<bool>(false)),
		m_requeueToken(new std::atomic<bool>(false))
		{
		}

		virtual void DoTask()
		{
			CookVoiceService::instance().executePreparedVoiceJob(m_voiceId, m_cancelToken, m_requeueToken);
		}

		virtual void StopTask()
		{
			m_cancelToken->store(true);
		}

		virtual bool ReExecute()
		{
			m_requeueToken->store(true);
			return true;
		}

		virtual CTask* Clone()
		{
			// ReExecute uses this clone to restart the same voice job after a priority preemption.
			// Keep the business parameters and create a fresh cancellation token in the constructor.
			return new CookVoiceGenerateTask(m_voiceId);
		}

	private:
		std::string m_voiceId;
		std::shared_ptr<std::atomic<bool>> m_cancelToken;
		std::shared_ptr<std::atomic<bool>> m_requeueToken;
	};

struct CookVoiceServiceState
{
	CookVoiceServiceState() :
	m_tempSerial(0),
	m_nextWorkerIndex(0),
	m_workersInitialized(false)
	{
	}

	std::mutex m_mutex;
	uint64_t m_tempSerial;
	std::map<std::string, std::shared_ptr<CookVoicePreparedJob>> m_jobMap;
	std::map<std::string, std::shared_ptr<CookVoicePreparedPlan>> m_planMap;
	std::map<std::string, std::shared_ptr<std::mutex>> m_audioMutexMap;
	std::vector<uint32_t> m_workerThreadIds;
	size_t m_nextWorkerIndex;
	bool m_workersInitialized;
};

CookVoiceTtsResult::CookVoiceTtsResult() :
m_ok(false),
m_cacheHit(false),
m_audioBytes(0)
{
	m_provider = kVoiceProviderDisabled;
	m_format = kVoiceFormatWav;
	m_contentType = kVoiceContentTypeWav;
}

CookVoicePreparedItem::CookVoicePreparedItem() :
m_required(false),
m_order(0)
{
	m_status = kVoiceStatusPending;
}

CookVoicePrepareResult::CookVoicePrepareResult() :
m_ok(false),
m_requiredCount(0),
m_workerThreadCount(0)
{
}

CookVoiceFetchResult::CookVoiceFetchResult() :
m_ok(false),
m_ready(false)
{
	m_status = kVoiceStatusPending;
}

CookVoiceService& CookVoiceService::instance()
{
	static CookVoiceService s_service;
	return s_service;
}

CookVoiceService::CookVoiceService() :
m_state(new CookVoiceServiceState)
{

}

CookVoiceService::~CookVoiceService()
{
	shutdown();
	delete m_state;
	m_state = nullptr;
}

bool CookVoiceService::enabled() const
{
	std::string provider = configuredProvider();
	return provider == kVoiceProviderWindowsSapi ||
		provider == kVoiceProviderCommand ||
		provider == kVoiceProviderCommandWav ||
		provider == kVoiceProviderCommandMp3;
}

CookVoiceTtsResult CookVoiceService::synthesize(const std::string& textUtf8, const std::string& gender)
{
	return synthesizeInternal(textUtf8, gender, std::shared_ptr<std::atomic<bool>>());
}

CookVoiceTtsResult CookVoiceService::synthesizeInternal(const std::string& textUtf8, const std::string& gender, const std::shared_ptr<std::atomic<bool>>& cancelToken)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	CookVoiceTtsResult result;
	std::string provider = configuredProvider();
	result.m_provider = provider;
	result.m_format = audioFormatForProvider(provider);
	result.m_contentType = contentTypeForFormat(result.m_format);
	LOGINFO("CookVoiceService synthesize begin provider=%s gender=%s textBytes=%d",
	        provider.c_str(),
	        gender.c_str(),
	        static_cast<int32_t>(textUtf8.size()));
	if (!enabled())
	{
		result.m_message = "server tts disabled";
		LOGWARNING("CookVoiceService synthesize disabled provider=%s costMs=%d",
		           provider.c_str(),
		           CSystem::GetHighTickCountMilliRunTime(beginTime));
		return result;
	}
	if (isVoiceTaskCancelled(cancelToken))
	{
		result.m_message = "voice synthesize cancelled";
		LOGINFO("CookVoiceService synthesize cancelled before normalize provider=%s costMs=%d",
		        provider.c_str(),
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
		return result;
	}

	std::string normalizedText = normalizeText(textUtf8);
	if (normalizedText.empty())
	{
		result.m_message = "text is required";
		LOGWARNING("CookVoiceService synthesize failed reason=empty_text costMs=%d",
		           CSystem::GetHighTickCountMilliRunTime(beginTime));
		return result;
	}
	if (g_config.m_voiceMaxTextBytes > 0 && normalizedText.size() > static_cast<size_t>(g_config.m_voiceMaxTextBytes))
	{
		result.m_message = "text is too long";
		LOGWARNING("CookVoiceService synthesize failed reason=text_too_long textBytes=%d maxBytes=%d costMs=%d",
		           static_cast<int32_t>(normalizedText.size()),
		           g_config.m_voiceMaxTextBytes,
		           CSystem::GetHighTickCountMilliRunTime(beginTime));
		return result;
	}

	std::string normalizedGender = normalizeGender(gender);
	result.m_audioKey = buildAudioKey(normalizedText, normalizedGender);
	result.m_filePath = audioFilePath(result.m_audioKey);
	if (isVoiceTaskCancelled(cancelToken))
	{
		result.m_message = "voice synthesize cancelled";
		LOGINFO("CookVoiceService synthesize cancelled before cache audioKey=%s costMs=%d",
		        result.m_audioKey.c_str(),
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
		return result;
	}

	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		int32_t cachedBytes = fileSizeBytes(result.m_filePath);
		if (cachedBytes > 0)
		{
			result.m_ok = true;
			result.m_cacheHit = true;
			result.m_audioBytes = cachedBytes;
			result.m_message = "cache hit";
			LOGINFO("CookVoiceService synthesize cache hit audioKey=%s audioBytes=%d costMs=%d",
			        result.m_audioKey.c_str(),
			        result.m_audioBytes,
			        CSystem::GetHighTickCountMilliRunTime(beginTime));
			return result;
		}

		if (!ensureDirectoryTree(cacheDirectoryPath()))
		{
			result.m_message = "voice cache directory create failed";
			LOGERROR("CookVoiceService synthesize failed reason=cache_directory path=%s costMs=%d",
			         cacheDirectoryPath().c_str(),
			         CSystem::GetHighTickCountMilliRunTime(beginTime));
			return result;
		}
	}

	std::string tempPath;
	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		++m_state->m_tempSerial;
		std::ostringstream oss;
		oss << result.m_filePath << ".tmp." << m_state->m_tempSerial << "." << result.m_format;
		tempPath = oss.str();
		if (CSystem::DirOrFileExist(tempPath))
		{
			CSystem::deleteFile(tempPath.c_str());
		}
	}

	std::shared_ptr<std::mutex> generateMutex = audioMutexForKey(result.m_audioKey);
	{
		std::chrono::high_resolution_clock::time_point waitBeginTime = CSystem::GetHighTickCount();
		std::unique_lock<std::mutex> generateLock(*generateMutex, std::defer_lock);
		while (!generateLock.try_lock())
		{
			if (isVoiceTaskCancelled(cancelToken))
			{
				CSystem::deleteFile(tempPath.c_str());
				result.m_message = "voice synthesize cancelled";
				LOGINFO("CookVoiceService synthesize cancelled waiting generate lock audioKey=%s costMs=%d",
				        result.m_audioKey.c_str(),
				        CSystem::GetHighTickCountMilliRunTime(beginTime));
				return result;
			}
			CSystem::Sleep(20);
		}
		int32_t waitMs = CSystem::GetHighTickCountMilliRunTime(waitBeginTime);
		LOGINFO("CookVoiceService synthesize generate lock audioKey=%s waitMs=%d",
		        result.m_audioKey.c_str(),
		        waitMs);

		{
			std::lock_guard<std::mutex> lock(m_state->m_mutex);
			int32_t cachedBytes = fileSizeBytes(result.m_filePath);
			if (cachedBytes > 0)
			{
				CSystem::deleteFile(tempPath.c_str());
				result.m_audioBytes = cachedBytes;
				result.m_ok = true;
				result.m_cacheHit = true;
				result.m_message = "cache hit before synthesize";
				LOGINFO("CookVoiceService synthesize cache hit after wait audioKey=%s audioBytes=%d waitMs=%d costMs=%d",
				        result.m_audioKey.c_str(),
				        result.m_audioBytes,
				        waitMs,
				        CSystem::GetHighTickCountMilliRunTime(beginTime));
				return result;
			}
		}

		std::string message;
		if (isVoiceTaskCancelled(cancelToken))
		{
			CSystem::deleteFile(tempPath.c_str());
			result.m_message = "voice synthesize cancelled";
			LOGINFO("CookVoiceService synthesize cancelled before generate audioKey=%s waitMs=%d costMs=%d",
			        result.m_audioKey.c_str(),
			        waitMs,
			        CSystem::GetHighTickCountMilliRunTime(beginTime));
			return result;
		}
		if (!generateAudioFile(normalizedText, normalizedGender, tempPath, message, cancelToken))
		{
			CSystem::deleteFile(tempPath.c_str());
			result.m_message = message.empty() ? "voice synthesize failed" : message;
			LOGERROR("CookVoiceService synthesize generate failed provider=%s audioKey=%s message=%s waitMs=%d costMs=%d",
			         provider.c_str(),
			         result.m_audioKey.c_str(),
			         result.m_message.c_str(),
			         waitMs,
			         CSystem::GetHighTickCountMilliRunTime(beginTime));
			return result;
		}
		if (isVoiceTaskCancelled(cancelToken))
		{
			CSystem::deleteFile(tempPath.c_str());
			result.m_message = "voice synthesize cancelled";
			LOGINFO("CookVoiceService synthesize cancelled after generate audioKey=%s waitMs=%d costMs=%d",
			        result.m_audioKey.c_str(),
			        waitMs,
			        CSystem::GetHighTickCountMilliRunTime(beginTime));
			return result;
		}

		{
			std::lock_guard<std::mutex> lock(m_state->m_mutex);
			if (std::rename(tempPath.c_str(), result.m_filePath.c_str()) != 0)
			{
				CSystem::deleteFile(tempPath.c_str());
				result.m_message = "voice cache file save failed";
				LOGERROR("CookVoiceService synthesize save failed audioKey=%s tempPath=%s filePath=%s costMs=%d",
				         result.m_audioKey.c_str(),
				         tempPath.c_str(),
				         result.m_filePath.c_str(),
				         CSystem::GetHighTickCountMilliRunTime(beginTime));
				return result;
			}
			result.m_audioBytes = fileSizeBytes(result.m_filePath);
		}
	}

	result.m_ok = result.m_format == kVoiceFormatWav ? result.m_audioBytes > 44 : result.m_audioBytes > 0;
	result.m_message = result.m_ok ? "ok" : "voice file is empty";
	if (result.m_ok)
	{
		LOGINFO("CookVoiceService synthesize success provider=%s audioKey=%s audioBytes=%d costMs=%d",
		        provider.c_str(),
		        result.m_audioKey.c_str(),
		        result.m_audioBytes,
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
	}
	else
	{
		LOGERROR("CookVoiceService synthesize empty file provider=%s audioKey=%s audioBytes=%d costMs=%d",
		         provider.c_str(),
		         result.m_audioKey.c_str(),
		         result.m_audioBytes,
		         CSystem::GetHighTickCountMilliRunTime(beginTime));
	}
	return result;
}

CookVoicePrepareResult CookVoiceService::prepareVoices(const std::string& planId, const std::vector<std::string>& texts, const std::string& gender, int32_t requiredCount)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	CookVoicePrepareResult result;
	result.m_ok = true;
	result.m_message = "ok";
	result.m_planId = planId;
	result.m_gender = normalizeGender(gender);
	result.m_workerThreadCount = g_config.m_voiceWorkerThreadCount;

	std::vector<std::string> normalizedTexts;
	std::set<std::string> textSet;
	for (size_t i = 0; i < texts.size(); ++i)
	{
		std::string text = normalizeText(texts[i]);
		if (text.empty() || textSet.find(text) != textSet.end())
		{
			continue;
		}
		textSet.insert(text);
		normalizedTexts.push_back(text);
	}
	result.m_requiredCount = std::max(0, std::min(requiredCount, static_cast<int32_t>(normalizedTexts.size())));
	if (normalizedTexts.empty())
	{
		result.m_ok = false;
		result.m_message = "voice texts are empty";
		return result;
	}

	ensureVoiceWorkers();
	std::shared_ptr<CookVoicePreparedPlan> plan(new CookVoicePreparedPlan);

	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		if (planId.empty())
		{
			++m_state->m_tempSerial;
			plan->m_planId = "voice_plan_" +
				CStringManager::toStringInt32(static_cast<int32_t>(std::time(nullptr))) +
				"_" +
				CStringManager::toStringInt64(static_cast<int64_t>(m_state->m_tempSerial));
		}
		else
		{
			plan->m_planId = planId;
		}
		for (size_t i = 0; i < normalizedTexts.size(); ++i)
		{
			bool required = static_cast<int32_t>(i) < result.m_requiredCount;
			std::string voiceId = buildVoiceId(normalizedTexts[i], result.m_gender);
			std::shared_ptr<CookVoicePreparedJob> job;
			std::map<std::string, std::shared_ptr<CookVoicePreparedJob>>::iterator it = m_state->m_jobMap.find(voiceId);
			if (it == m_state->m_jobMap.end())
			{
				job.reset(new CookVoicePreparedJob);
				job->m_voiceId = voiceId;
				job->m_text = normalizedTexts[i];
				job->m_gender = result.m_gender;
				job->m_required = required;
				job->m_order = static_cast<int32_t>(i);
				m_state->m_jobMap[voiceId] = job;
			}
			else
			{
				job = it->second;
				if (required)
				{
					job->m_required = true;
				}
				if (job->m_order == 0 && i > 0)
				{
					job->m_order = static_cast<int32_t>(i);
				}
			}

			CookVoicePreparedItem item;
			item.m_voiceId = voiceId;
			item.m_text = normalizedTexts[i];
			item.m_status = job->m_status;
			item.m_required = required;
			item.m_order = static_cast<int32_t>(i);
			result.m_items.push_back(item);
			if (required)
			{
				result.m_requiredVoiceIds.push_back(voiceId);
				plan->m_requiredVoiceIds.push_back(voiceId);
				enqueuePreparedVoiceNoLock(voiceId, kVoiceRequiredPriority);
			}
			else
			{
				result.m_backgroundVoiceIds.push_back(voiceId);
				plan->m_backgroundVoiceIds.push_back(voiceId);
			}
		}
		m_state->m_planMap[plan->m_planId] = plan;
		queueBackgroundForReadyPlansNoLock();
	}

	LOGINFO("CookVoiceService prepare voices planId=%s gender=%s total=%d required=%d background=%d workers=%d costMs=%d",
	        plan->m_planId.c_str(),
	        result.m_gender.c_str(),
	        static_cast<int32_t>(result.m_items.size()),
	        static_cast<int32_t>(result.m_requiredVoiceIds.size()),
	        static_cast<int32_t>(result.m_backgroundVoiceIds.size()),
	        result.m_workerThreadCount,
	        CSystem::GetHighTickCountMilliRunTime(beginTime));
	result.m_planId = plan->m_planId;
	return result;
}

CookVoiceFetchResult CookVoiceService::fetchPreparedVoice(const std::string& voiceId, int32_t waitMilliseconds)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	CookVoiceFetchResult result;
	result.m_voiceId = voiceId;
	if (voiceId.empty())
	{
		result.m_status = kVoiceStatusFailed;
		result.m_message = "voiceId is required";
		return result;
	}

	std::shared_ptr<CookVoicePreparedJob> job;
	{
		std::unique_lock<std::mutex> lock(m_state->m_mutex);
		std::map<std::string, std::shared_ptr<CookVoicePreparedJob>>::iterator it = m_state->m_jobMap.find(voiceId);
		if (it == m_state->m_jobMap.end())
		{
			result.m_status = kVoiceStatusFailed;
			result.m_message = "prepared voice not found";
			return result;
		}
		job = it->second;
		result.m_text = job->m_text;
		if (job->m_status == kVoiceStatusPending)
		{
			enqueuePreparedVoiceNoLock(voiceId, job->m_required ? kVoiceRequiredPriority : kVoiceNormalPriority);
		}
		int32_t safeWait = std::max(0, waitMilliseconds);
		if (safeWait > 0 && job->m_status != kVoiceStatusReady && job->m_status != kVoiceStatusFailed)
		{
			job->m_condition.wait_for(lock, std::chrono::milliseconds(safeWait));
		}
		result.m_status = job->m_status;
		result.m_message = job->m_message;
		result.m_tts = job->m_result;
	}

	result.m_ready = result.m_status == kVoiceStatusReady && result.m_tts.m_ok;
	result.m_ok = result.m_ready;
	if (result.m_ready)
	{
		result.m_message = "ok";
		LOGINFO("CookVoiceService fetch ready voiceId=%s audioKey=%s cacheHit=%d audioBytes=%d costMs=%d",
		        voiceId.c_str(),
		        result.m_tts.m_audioKey.c_str(),
		        result.m_tts.m_cacheHit ? 1 : 0,
		        result.m_tts.m_audioBytes,
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
	}
	else
	{
		LOGINFO("CookVoiceService fetch not ready voiceId=%s status=%s message=%s costMs=%d",
		        voiceId.c_str(),
		        result.m_status.c_str(),
		        result.m_message.c_str(),
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
	}
	return result;
}

void CookVoiceService::executePreparedVoiceJob(const std::string& voiceId, const std::shared_ptr<std::atomic<bool>>& cancelToken, const std::shared_ptr<std::atomic<bool>>& requeueToken)
{
	std::shared_ptr<CookVoicePreparedJob> job;
	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		std::map<std::string, std::shared_ptr<CookVoicePreparedJob>>::iterator it = m_state->m_jobMap.find(voiceId);
		if (it == m_state->m_jobMap.end())
		{
			return;
		}
		job = it->second;
		if (job->m_status == kVoiceStatusReady)
		{
			job->m_queued = false;
			job->m_condition.notify_all();
			return;
		}
		if (isVoiceTaskCancelled(cancelToken))
		{
			job->m_queued = false;
			job->m_condition.notify_all();
			return;
		}
		job->m_status = kVoiceStatusRunning;
		job->m_message = "running";
	}

	LOGINFO("CookVoiceService job begin voiceId=%s required=%d textBytes=%d",
	        job->m_voiceId.c_str(),
	        job->m_required ? 1 : 0,
	        static_cast<int32_t>(job->m_text.size()));
	CookVoiceTtsResult tts = synthesizeInternal(job->m_text, job->m_gender, cancelToken);
	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		if (isVoiceTaskCancelled(cancelToken))
		{
			job->m_status = kVoiceStatusPending;
			if (requeueToken != nullptr && requeueToken->load())
			{
				job->m_message = "cancelled and requeued";
				job->m_queued = true;
			}
			else
			{
				job->m_message = "cancelled";
				job->m_queued = false;
			}
			job->m_condition.notify_all();
			LOGINFO("CookVoiceService job cancelled voiceId=%s required=%d",
			        job->m_voiceId.c_str(),
			        job->m_required ? 1 : 0);
			return;
		}
		job->m_result = tts;
		job->m_status = tts.m_ok ? kVoiceStatusReady : kVoiceStatusFailed;
		job->m_message = tts.m_ok ? "ok" : tts.m_message;
		job->m_queued = false;
		job->m_condition.notify_all();
		queueBackgroundForReadyPlansNoLock();
	}
	LOGINFO("CookVoiceService job finish voiceId=%s status=%s audioKey=%s audioBytes=%d",
	        job->m_voiceId.c_str(),
	        tts.m_ok ? kVoiceStatusReady : kVoiceStatusFailed,
	        tts.m_audioKey.c_str(),
	        tts.m_audioBytes);
}

void CookVoiceService::shutdown()
{
	if (m_state == nullptr)
	{
		return;
	}

	std::vector<uint32_t> workerThreadIds;
	{
		std::lock_guard<std::mutex> lock(m_state->m_mutex);
		workerThreadIds.swap(m_state->m_workerThreadIds);
		m_state->m_workersInitialized = false;
		m_state->m_nextWorkerIndex = 0;
		m_state->m_planMap.clear();
		m_state->m_jobMap.clear();
		m_state->m_audioMutexMap.clear();
	}

	for (size_t i = 0; i < workerThreadIds.size(); ++i)
	{
		CTaskThreadManager::Instance().Uninit(workerThreadIds[i]);
	}
	if (!workerThreadIds.empty())
	{
		LOGINFO("CookVoiceService shutdown workers=%d", static_cast<int32_t>(workerThreadIds.size()));
	}
}

void CookVoiceService::ensureVoiceWorkers()
{
	std::lock_guard<std::mutex> lock(m_state->m_mutex);
	if (m_state->m_workersInitialized)
	{
		return;
	}
	int32_t workerCount = std::max(1, g_config.m_voiceWorkerThreadCount);
	for (int32_t i = 0; i < workerCount; ++i)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		if (threadId != 0)
		{
			m_state->m_workerThreadIds.push_back(threadId);
		}
	}
	m_state->m_workersInitialized = true;
	LOGINFO("CookVoiceService workers initialized requested=%d actual=%d",
	        workerCount,
	        static_cast<int32_t>(m_state->m_workerThreadIds.size()));
}

bool CookVoiceService::enqueuePreparedVoiceNoLock(const std::string& voiceId, int32_t priority)
{
	std::map<std::string, std::shared_ptr<CookVoicePreparedJob>>::iterator jobIt = m_state->m_jobMap.find(voiceId);
	if (jobIt == m_state->m_jobMap.end() || jobIt->second == nullptr)
	{
		return false;
	}
	std::shared_ptr<CookVoicePreparedJob> job = jobIt->second;
	if (job->m_status == kVoiceStatusReady || job->m_status == kVoiceStatusRunning || job->m_queued)
	{
		return false;
	}
	if (m_state->m_workerThreadIds.empty())
	{
		return false;
	}
	size_t workerIndex = m_state->m_nextWorkerIndex++ % m_state->m_workerThreadIds.size();
	uint32_t threadId = m_state->m_workerThreadIds[workerIndex];
	std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	if (thread == nullptr)
	{
		return false;
	}
	job->m_status = kVoiceStatusPending;
	job->m_message = "queued";
	job->m_queued = true;
	job->m_priority = priority;
	std::shared_ptr<CTask> task(new CookVoiceGenerateTask(voiceId));
	thread->PostTask(task, priority);
	LOGINFO("CookVoiceService enqueue voiceId=%s priority=%d threadId=%u",
	        voiceId.c_str(),
	        priority,
	        threadId);
	return true;
}

void CookVoiceService::queueBackgroundForReadyPlansNoLock()
{
	for (std::map<std::string, std::shared_ptr<CookVoicePreparedPlan>>::iterator it = m_state->m_planMap.begin(); it != m_state->m_planMap.end(); ++it)
	{
		std::shared_ptr<CookVoicePreparedPlan> plan = it->second;
		if (plan == nullptr || plan->m_backgroundQueued)
		{
			continue;
		}
		bool requiredReady = true;
		for (size_t i = 0; i < plan->m_requiredVoiceIds.size(); ++i)
		{
			std::map<std::string, std::shared_ptr<CookVoicePreparedJob>>::iterator jobIt = m_state->m_jobMap.find(plan->m_requiredVoiceIds[i]);
			if (jobIt == m_state->m_jobMap.end() || jobIt->second == nullptr || jobIt->second->m_status != kVoiceStatusReady)
			{
				requiredReady = false;
				break;
			}
		}
		if (!requiredReady)
		{
			continue;
		}
		plan->m_backgroundQueued = true;
		for (size_t i = 0; i < plan->m_backgroundVoiceIds.size(); ++i)
		{
			enqueuePreparedVoiceNoLock(plan->m_backgroundVoiceIds[i], kVoiceNormalPriority);
		}
		LOGINFO("CookVoiceService background queued planId=%s count=%d",
		        plan->m_planId.c_str(),
		        static_cast<int32_t>(plan->m_backgroundVoiceIds.size()));
	}
}

std::shared_ptr<std::mutex> CookVoiceService::audioMutexForKey(const std::string& audioKey)
{
	std::lock_guard<std::mutex> lock(m_state->m_mutex);
	std::map<std::string, std::shared_ptr<std::mutex>>::iterator it = m_state->m_audioMutexMap.find(audioKey);
	if (it != m_state->m_audioMutexMap.end())
	{
		return it->second;
	}
	std::shared_ptr<std::mutex> mutex(new std::mutex);
	m_state->m_audioMutexMap[audioKey] = mutex;
	return mutex;
}

bool CookVoiceService::resolveAudioFile(const std::string& audioKey, std::string& filePath, std::string& contentType, std::string& format) const
{
	if (!isSafeAudioKey(audioKey))
	{
		return false;
	}
	filePath = audioFilePath(audioKey);
	if (fileSizeBytes(filePath) <= 0)
	{
		return false;
	}
	format = audioKey.size() >= 4 && audioKey.substr(audioKey.size() - 4) == ".mp3" ? kVoiceFormatMp3 : kVoiceFormatWav;
	contentType = contentTypeForFormat(format);
	return true;
}

std::string CookVoiceService::cacheDirectoryPath() const
{
	return CookHelper::appendPathSeparator(CSystem::GetCurrentExePath() + g_config.m_voiceCacheDirectoryName);
}

std::string CookVoiceService::audioFilePath(const std::string& audioKey) const
{
	return cacheDirectoryPath() + audioKey;
}

bool CookVoiceService::ensureDirectoryTree(const std::string& directoryPath) const
{
	std::string path = CookHelper::appendPathSeparator(directoryPath);
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
		size_t end = path.find(CookHelper::pathSeparator(), start);
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

std::string CookVoiceService::normalizeText(const std::string& textUtf8) const
{
	std::string text = textUtf8;
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (text[i] == '\r' || text[i] == '\n' || text[i] == '\t')
		{
			text[i] = ' ';
		}
	}
	size_t begin = 0;
	while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])))
	{
		++begin;
	}
	size_t end = text.size();
	while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
	{
		--end;
	}
	return text.substr(begin, end - begin);
}

std::string CookVoiceService::normalizeGender(const std::string& gender) const
{
	std::string lower = CookHelper::lowerAscii(gender);
	return lower == "male" ? "male" : "female";
}

std::string CookVoiceService::buildAudioKey(const std::string& textUtf8, const std::string& gender) const
{
	std::string provider = configuredProvider();
	std::string format = audioFormatForProvider(provider);
	return gender + "_" + CookHelper::hashHex128(provider + "|" + format + "|" + gender + "|" + textUtf8) + "." + format;
}

std::string CookVoiceService::buildVoiceId(const std::string& textUtf8, const std::string& gender) const
{
	std::string normalizedText = normalizeText(textUtf8);
	std::string normalizedGender = normalizeGender(gender);
	if (normalizedText.empty())
	{
		return "";
	}
	return "voice_" + CookHelper::hashHex128(configuredProvider() + "|" + normalizedGender + "|" + normalizedText);
}

std::string CookVoiceService::configuredProvider() const
{
	std::string provider = CookHelper::lowerAscii(g_config.m_voiceTtsProvider);
	if (provider.empty() || provider == "none" || provider == "off")
	{
		return kVoiceProviderDisabled;
	}
	return provider;
}

std::string CookVoiceService::audioFormatForProvider(const std::string& provider) const
{
	std::string normalizedProvider = CookHelper::lowerAscii(provider);
	if (normalizedProvider == kVoiceProviderCommandMp3)
	{
		return kVoiceFormatMp3;
	}
	if (normalizedProvider == kVoiceProviderCommand)
	{
		std::string configuredFormat = CookHelper::lowerAscii(g_config.m_voiceCommandAudioFormat);
		return configuredFormat == kVoiceFormatWav ? kVoiceFormatWav : kVoiceFormatMp3;
	}
	return kVoiceFormatWav;
}

std::string CookVoiceService::contentTypeForFormat(const std::string& format) const
{
	return CookHelper::lowerAscii(format) == kVoiceFormatMp3 ? kVoiceContentTypeMp3 : kVoiceContentTypeWav;
}

bool CookVoiceService::isSafeAudioKey(const std::string& audioKey) const
{
	if (audioKey.size() < 42 || audioKey.size() > 80)
	{
		return false;
	}
	if (audioKey.find("..") != std::string::npos)
	{
		return false;
	}
	if (audioKey.size() < 4)
	{
		return false;
	}
	std::string extension = audioKey.substr(audioKey.size() - 4);
	if (extension != ".wav" && extension != ".mp3")
	{
		return false;
	}
	for (size_t i = 0; i < audioKey.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(audioKey[i]);
		if (std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.')
		{
			continue;
		}
		return false;
	}
	return true;
}

bool CookVoiceService::isVoiceTaskCancelled(const std::shared_ptr<std::atomic<bool>>& cancelToken) const
{
	return cancelToken != nullptr && cancelToken->load();
}

int32_t CookVoiceService::fileSizeBytes(const std::string& filePath) const
{
	std::string content = CSystem::readFile(filePath);
	if (content.empty())
	{
		return 0;
	}
	return content.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()) ? 0 : static_cast<int32_t>(content.size());
}

bool CookVoiceService::generateAudioFile(const std::string& textUtf8, const std::string& gender, const std::string& filePath, std::string& message, const std::shared_ptr<std::atomic<bool>>& cancelToken) const
{
	if (isVoiceTaskCancelled(cancelToken))
	{
		message = "voice synthesize cancelled";
		return false;
	}
	std::string provider = configuredProvider();
	if (provider == kVoiceProviderWindowsSapi)
	{
#ifdef _WIN32
		return generateWindowsSapiWav(textUtf8, gender, filePath, message, cancelToken);
#else
		message = "windows_sapi provider only works on Windows";
		return false;
#endif
	}
	if (provider == kVoiceProviderCommand || provider == kVoiceProviderCommandWav || provider == kVoiceProviderCommandMp3)
	{
		return generateCommandAudioFile(textUtf8, gender, filePath, audioFormatForProvider(provider), message, cancelToken);
	}
	message = "unsupported voice tts provider: " + provider;
	return false;
}

bool CookVoiceService::generateCommandAudioFile(const std::string& textUtf8, const std::string& gender, const std::string& filePath, const std::string& format, std::string& message, const std::shared_ptr<std::atomic<bool>>& cancelToken) const
{
	if (g_config.m_voiceCommandTemplate.empty())
	{
		message = "command tts provider needs Voice.CommandTemplate";
		return false;
	}
	if (isVoiceTaskCancelled(cancelToken))
	{
		message = "voice synthesize cancelled";
		return false;
	}

	std::string textFilePath = filePath + ".txt";
	CSystem::saveFile(textUtf8, textFilePath);
	std::string command = g_config.m_voiceCommandTemplate;
	CStringManager::Replace(command, "{textFile}", CookHelper::quoteCommandArgument(textFilePath));
	CStringManager::Replace(command, "{audioFile}", CookHelper::quoteCommandArgument(filePath));
	CStringManager::Replace(command, "{gender}", gender);
	CStringManager::Replace(command, "{format}", format);
	CStringManager::Replace(command, "{text}", CookHelper::quoteCommandArgument(textUtf8));

	std::string output;
	int32_t commandResult = -1;
#ifdef _WIN32
	if (!runCommandCancelable(command, g_config.m_voiceCommandUseShell, cancelToken, output, commandResult, message))
	{
		CSystem::deleteFile(textFilePath.c_str());
		return false;
	}
#else
	commandResult = CSystem::SystemCommand(command, output, g_config.m_voiceCommandUseShell);
#endif
	CSystem::deleteFile(textFilePath.c_str());
	if (isVoiceTaskCancelled(cancelToken))
	{
		message = "voice synthesize cancelled";
		return false;
	}
	int32_t audioBytes = fileSizeBytes(filePath);
	bool validAudio = format == kVoiceFormatWav ? audioBytes > 44 : audioBytes > 0;
	if (commandResult != 0 || !validAudio)
	{
		std::string outputText = CookHelper::shortenCommandOutput(output);
		message = "command tts failed result=" + CStringManager::toStringInt32(commandResult);
		if (!outputText.empty())
		{
			message += " output=" + outputText;
		}
		return false;
	}
	return true;
}

#ifdef _WIN32
bool CookVoiceService::runCommandCancelable(const std::string& command, bool useShell, const std::shared_ptr<std::atomic<bool>>& cancelToken, std::string& output, int32_t& exitCode, std::string& message) const
{
	output.clear();
	exitCode = -1;
	message.clear();
	if (command.empty())
	{
		message = "command is empty";
		return false;
	}
	if (isVoiceTaskCancelled(cancelToken))
	{
		message = "voice synthesize cancelled";
		return false;
	}

	SECURITY_ATTRIBUTES securityAttributes;
	HANDLE readHandle = nullptr;
	HANDLE writeHandle = nullptr;
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.lpSecurityDescriptor = nullptr;
	securityAttributes.bInheritHandle = TRUE;
	if (!::CreatePipe(&readHandle, &writeHandle, &securityAttributes, 0))
	{
		message = "CreatePipe failed";
		return false;
	}
	::SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA startupInfo = { 0 };
	PROCESS_INFORMATION processInfo = { 0 };
	startupInfo.cb = sizeof(STARTUPINFOA);
	startupInfo.hStdInput = ::GetStdHandle(STD_INPUT_HANDLE);
	startupInfo.hStdError = writeHandle;
	startupInfo.hStdOutput = writeHandle;
	startupInfo.wShowWindow = SW_HIDE;
	startupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

	std::string processCommand = useShell ? "cmd.exe /C " + command : command;
	std::vector<char> commandBuffer(processCommand.begin(), processCommand.end());
	commandBuffer.push_back('\0');
	if (!::CreateProcessA(nullptr, &commandBuffer[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo))
	{
		::CloseHandle(readHandle);
		::CloseHandle(writeHandle);
		message = "CreateProcess failed";
		return false;
	}
	::CloseHandle(writeHandle);
	writeHandle = nullptr;

	bool cancelled = false;
	bool processEnded = false;
	char buffer[4096] = { 0 };
	while (!processEnded)
	{
		if (isVoiceTaskCancelled(cancelToken))
		{
			cancelled = true;
			::TerminateProcess(processInfo.hProcess, 1);
			::WaitForSingleObject(processInfo.hProcess, 1000);
			break;
		}

		DWORD availableBytes = 0;
		if (::PeekNamedPipe(readHandle, nullptr, 0, nullptr, &availableBytes, nullptr) && availableBytes > 0)
		{
			DWORD bytesToRead = std::min<DWORD>(availableBytes, static_cast<DWORD>(sizeof(buffer) - 1));
			DWORD bytesRead = 0;
			if (::ReadFile(readHandle, buffer, bytesToRead, &bytesRead, nullptr) && bytesRead > 0)
			{
				buffer[bytesRead] = '\0';
				output.append(buffer, bytesRead);
				continue;
			}
		}

		DWORD waitResult = ::WaitForSingleObject(processInfo.hProcess, 20);
		if (waitResult == WAIT_OBJECT_0)
		{
			processEnded = true;
		}
		else if (waitResult != WAIT_TIMEOUT)
		{
			processEnded = true;
		}
	}

	while (true)
	{
		DWORD availableBytes = 0;
		if (!::PeekNamedPipe(readHandle, nullptr, 0, nullptr, &availableBytes, nullptr) || availableBytes == 0)
		{
			break;
		}
		DWORD bytesToRead = std::min<DWORD>(availableBytes, static_cast<DWORD>(sizeof(buffer) - 1));
		DWORD bytesRead = 0;
		if (!::ReadFile(readHandle, buffer, bytesToRead, &bytesRead, nullptr) || bytesRead == 0)
		{
			break;
		}
		buffer[bytesRead] = '\0';
		output.append(buffer, bytesRead);
	}

	DWORD processExitCode = 0;
	if (::GetExitCodeProcess(processInfo.hProcess, &processExitCode))
	{
		exitCode = static_cast<int32_t>(processExitCode);
	}
	::CloseHandle(readHandle);
	::CloseHandle(processInfo.hThread);
	::CloseHandle(processInfo.hProcess);

	if (cancelled)
	{
		exitCode = -2;
		message = "voice synthesize cancelled";
		return false;
	}
	return true;
}
#endif

#ifdef _WIN32
bool CookVoiceService::generateWindowsSapiWav(const std::string& textUtf8, const std::string& gender, const std::string& filePath, std::string& message, const std::shared_ptr<std::atomic<bool>>& cancelToken) const
{
	if (isVoiceTaskCancelled(cancelToken))
	{
		message = "voice synthesize cancelled";
		return false;
	}
	HRESULT initHr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	bool needUninit = SUCCEEDED(initHr);
	if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE)
	{
		message = "CoInitializeEx failed " + hresultText(initHr);
		return false;
	}

	ISpVoice* voice = nullptr;
	ISpStream* stream = nullptr;
	HRESULT hr = ::CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, reinterpret_cast<void**>(&voice));
	if (SUCCEEDED(hr) && voice != nullptr)
	{
		trySelectVoiceByGender(voice, gender);
		voice->SetVolume(100);
		voice->SetRate(gender == "male" ? -1 : 0);
		hr = ::CoCreateInstance(CLSID_SpStream, nullptr, CLSCTX_ALL, IID_ISpStream, reinterpret_cast<void**>(&stream));
	}
	if (SUCCEEDED(hr) && stream != nullptr)
	{
		WAVEFORMATEX waveFormat;
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = 1;
		waveFormat.nSamplesPerSec = 16000;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = static_cast<WORD>(waveFormat.nChannels * waveFormat.wBitsPerSample / 8);
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;
		std::wstring widePath = CStringManager::AnsiToUnicode(filePath);
		hr = stream->BindToFile(widePath.c_str(), SPFM_CREATE_ALWAYS, &SPDFID_WaveFormatEx, &waveFormat, 0);
	}
	if (SUCCEEDED(hr) && voice != nullptr && stream != nullptr)
	{
		std::wstring wideText = CStringManager::Utf8ToUnicode(textUtf8);
		if (wideText.empty() && !textUtf8.empty())
		{
			wideText = CStringManager::AnsiToUnicode(textUtf8);
		}
		hr = voice->SetOutput(stream, TRUE);
		if (SUCCEEDED(hr))
		{
			hr = voice->Speak(wideText.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
		}
		if (SUCCEEDED(hr))
		{
			while (true)
			{
				HRESULT waitHr = voice->WaitUntilDone(20);
				if (waitHr == S_OK)
				{
					break;
				}
				if (FAILED(waitHr))
				{
					hr = waitHr;
					break;
				}
				if (isVoiceTaskCancelled(cancelToken))
				{
					voice->Speak(L"", SPF_ASYNC | SPF_PURGEBEFORESPEAK, nullptr);
					voice->WaitUntilDone(1000);
					message = "voice synthesize cancelled";
					hr = E_ABORT;
					break;
				}
			}
		}
	}
	if (stream != nullptr)
	{
		stream->Close();
	}
	releaseCom(stream);
	releaseCom(voice);
	if (needUninit)
	{
		::CoUninitialize();
	}
	if (FAILED(hr))
	{
		message = "Windows SAPI synthesize failed " + hresultText(hr);
		return false;
	}
	return fileSizeBytes(filePath) > 44;
}
#endif
