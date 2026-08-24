#pragma once
#include <atomic>
#include <stdint.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct CookVoiceServiceState;

/** One server-side text-to-speech result.
*/
struct CookVoiceTtsResult
{
	/** Constructor.
	*/
	CookVoiceTtsResult();

	// True when audio was generated or found in cache.
	bool m_ok;
	// Human-readable result message.
	std::string m_message;
	// Actual provider used by this result.
	std::string m_provider;
	// Cache key used by the audio download endpoint.
	std::string m_audioKey;
	// Audio format. The Windows SAPI provider currently writes wav.
	std::string m_format;
	// HTTP Content-Type for the audio file.
	std::string m_contentType;
	// Absolute cached audio file path.
	std::string m_filePath;
	// True when an existing cache file was used.
	bool m_cacheHit;
	// Audio file byte count.
	int32_t m_audioBytes;
};

/** One prepared voice item returned to clients.
*/
struct CookVoicePreparedItem
{
	CookVoicePreparedItem();

	std::string m_voiceId;
	std::string m_text;
	std::string m_status;
	bool m_required;
	int32_t m_order;
};

/** Result of preparing a plan's voice list.
*/
struct CookVoicePrepareResult
{
	CookVoicePrepareResult();

	bool m_ok;
	std::string m_message;
	std::string m_planId;
	std::string m_gender;
	int32_t m_requiredCount;
	int32_t m_workerThreadCount;
	std::vector<CookVoicePreparedItem> m_items;
	std::vector<std::string> m_requiredVoiceIds;
	std::vector<std::string> m_backgroundVoiceIds;
};

/** Result of fetching a prepared voice item.
*/
struct CookVoiceFetchResult
{
	CookVoiceFetchResult();

	bool m_ok;
	bool m_ready;
	std::string m_status;
	std::string m_message;
	std::string m_voiceId;
	std::string m_text;
	CookVoiceTtsResult m_tts;
};

/** TTS service for cooking-step voice prompts.
*/
class CookVoiceService
{
public:
	/** Get global service instance.
	@return Service singleton.
	*/
	static CookVoiceService& instance();

	/** Generate audio or return a cached file.
	@param [in] textUtf8 UTF-8 text to speak.
	@param [in] gender female or male.
	@return TTS result.
	*/
	CookVoiceTtsResult synthesize(const std::string& textUtf8, const std::string& gender);

	/** Prepare a plan voice list and enqueue initial required voices first.
	@param [in] planId Plan id, may be empty.
	@param [in] texts Voice texts in client playback order.
	@param [in] gender female or male.
	@param [in] requiredCount Initial required voice item count.
	@return Prepared voice list and ids.
	*/
	CookVoicePrepareResult prepareVoices(const std::string& planId, const std::vector<std::string>& texts, const std::string& gender, int32_t requiredCount);

	/** Fetch a prepared voice item, waiting briefly when it is still being generated.
	@param [in] voiceId Prepared voice id returned by prepareVoices.
	@param [in] waitMilliseconds Maximum wait time in milliseconds.
	@return Voice status and generated audio information when ready.
	*/
	CookVoiceFetchResult fetchPreparedVoice(const std::string& voiceId, int32_t waitMilliseconds);

	/** Internal worker entry used by CTaskThreadManager tasks.
	@param [in] voiceId Prepared voice id.
	@param [in] cancelToken Task cancellation token.
	@param [in] requeueToken True when ReExecute has put a clone back into the queue.
	*/
	void executePreparedVoiceJob(const std::string& voiceId, const std::shared_ptr<std::atomic<bool>>& cancelToken, const std::shared_ptr<std::atomic<bool>>& requeueToken);

	/** Stop prepared-voice worker threads and clear pending runtime state.
	*/
	void shutdown();

	/** Resolve a cached audio file by key.
	@param [in] audioKey Audio key.
	@param [out] filePath Absolute audio file path.
	@param [out] contentType HTTP Content-Type.
	@param [out] format Audio format.
	@return True when the key is valid and the audio file exists.
	*/
	bool resolveAudioFile(const std::string& audioKey, std::string& filePath, std::string& contentType, std::string& format) const;

	/** Check whether server-side TTS is enabled.
	@return True when the configured provider is enabled.
	*/
	bool enabled() const;

private:
	/** Constructor.
	*/
	CookVoiceService();

	/** Destructor.
	*/
	~CookVoiceService();

	/** Copy is disabled.
	*/
	CookVoiceService(const CookVoiceService& other);

	/** Assignment is disabled.
	*/
	CookVoiceService& operator=(const CookVoiceService& other);

	/** Get absolute cache directory with trailing separator.
	@return Cache directory path.
	*/
	std::string cacheDirectoryPath() const;

	/** Build absolute audio file path.
	@param [in] audioKey Audio key.
	@return Audio file path.
	*/
	std::string audioFilePath(const std::string& audioKey) const;

	/** Ensure a directory tree exists.
	@param [in] directoryPath Directory path.
	@return True when the directory exists or was created.
	*/
	bool ensureDirectoryTree(const std::string& directoryPath) const;

	/** Normalize source text.
	@param [in] textUtf8 Raw UTF-8 text.
	@return Trimmed text.
	*/
	std::string normalizeText(const std::string& textUtf8) const;

	/** Normalize voice gender.
	@param [in] gender Requested gender.
	@return male or female.
	*/
	std::string normalizeGender(const std::string& gender) const;

	/** Build a stable cache key.
	@param [in] textUtf8 Normalized text.
	@param [in] gender Normalized gender.
	@return Safe file name key.
	*/
	std::string buildAudioKey(const std::string& textUtf8, const std::string& gender) const;

	/** Build a stable prepared voice id.
	@param [in] textUtf8 Raw UTF-8 text.
	@param [in] gender Requested gender.
	@return Voice id.
	*/
	std::string buildVoiceId(const std::string& textUtf8, const std::string& gender) const;

	/** Ensure worker threads for prepared voice jobs are running.
	*/
	void ensureVoiceWorkers();

	/** Enqueue one prepared voice job while m_state mutex is already held.
	@param [in] voiceId Prepared voice id.
	@param [in] priority Task priority.
	@return True if a task was posted.
	*/
	bool enqueuePreparedVoiceNoLock(const std::string& voiceId, int32_t priority);

	/** Queue normal-priority background voices for plans whose required voices are ready.
	*/
	void queueBackgroundForReadyPlansNoLock();

	/** Get per-audio-key generation mutex.
	@param [in] audioKey Audio key.
	@return Shared mutex for this audio key.
	*/
	std::shared_ptr<std::mutex> audioMutexForKey(const std::string& audioKey);

	/** Get normalized provider name from current config.
	@return Provider name.
	*/
	std::string configuredProvider() const;

	/** Get audio format for a provider.
	@param [in] provider Provider name.
	@return wav or mp3.
	*/
	std::string audioFormatForProvider(const std::string& provider) const;

	/** Get HTTP content type for an audio format.
	@param [in] format Audio format.
	@return Content type.
	*/
	std::string contentTypeForFormat(const std::string& format) const;

	/** Validate an audio key.
	@param [in] audioKey Audio key.
	@return True when the key is safe.
	*/
	bool isSafeAudioKey(const std::string& audioKey) const;

	/** Check whether a cancellable voice task has been stopped.
	@param [in] cancelToken Cancellation token.
	@return True when cancellation has been requested.
	*/
	bool isVoiceTaskCancelled(const std::shared_ptr<std::atomic<bool>>& cancelToken) const;

	/** Generate audio or return a cached file with optional cancellation.
	@param [in] textUtf8 UTF-8 text to speak.
	@param [in] gender female or male.
	@param [in] cancelToken Optional cancellation token.
	@return TTS result.
	*/
	CookVoiceTtsResult synthesizeInternal(const std::string& textUtf8, const std::string& gender, const std::shared_ptr<std::atomic<bool>>& cancelToken);

	/** Get file size.
	@param [in] filePath File path.
	@return File size in bytes, or 0 on failure.
	*/
	int32_t fileSizeBytes(const std::string& filePath) const;

	/** Generate an audio file through the configured provider.
	@param [in] textUtf8 Normalized text.
	@param [in] gender Normalized gender.
	@param [in] filePath Output audio path.
	@param [out] message Failure message.
	@return True when the file was generated.
	*/
	bool generateAudioFile(const std::string& textUtf8, const std::string& gender, const std::string& filePath, std::string& message, const std::shared_ptr<std::atomic<bool>>& cancelToken) const;

	/** Generate audio through a configured external command.
	@param [in] textUtf8 Normalized text.
	@param [in] gender Normalized gender.
	@param [in] filePath Output audio path.
	@param [in] format Output audio format.
	@param [out] message Failure message.
	@return True when the file was generated.
	*/
	bool generateCommandAudioFile(const std::string& textUtf8, const std::string& gender, const std::string& filePath, const std::string& format, std::string& message, const std::shared_ptr<std::atomic<bool>>& cancelToken) const;

#ifdef _WIN32
	/** Run an external TTS command while allowing cancellation.
	@param [in] command Command line.
	@param [in] useShell Whether to run through cmd.exe /C.
	@param [in] cancelToken Optional cancellation token.
	@param [out] output Captured process output.
	@param [out] exitCode Process exit code.
	@param [out] message Failure message.
	@return True when the process exits normally and was not cancelled.
	*/
	bool runCommandCancelable(const std::string& command, bool useShell, const std::shared_ptr<std::atomic<bool>>& cancelToken, std::string& output, int32_t& exitCode, std::string& message) const;
#endif

#ifdef _WIN32
	/** Generate wav through Windows SAPI.
	@param [in] textUtf8 Normalized text.
	@param [in] gender Normalized gender.
	@param [in] filePath Output wav path.
	@param [out] message Failure message.
	@return True when the file was generated.
	*/
	bool generateWindowsSapiWav(const std::string& textUtf8, const std::string& gender, const std::string& filePath, std::string& message, const std::shared_ptr<std::atomic<bool>>& cancelToken) const;
#endif

private:
	// Private runtime state. Keeps heavy standard-library headers out of this header.
	CookVoiceServiceState* m_state;
};
