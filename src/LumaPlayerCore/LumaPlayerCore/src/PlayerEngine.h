#pragma once
#include "LumaPlayerCoreConfig.h"
#include "IAudioRender.h"
#include "IVideoRender.h"
#include "MediaClock.h"
#include "PlayerCommand.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>

class FFmpegCppPlaybackReader;
class PlayerWorkerTask;
class PlayerPreviewTask;
class PlayerLoopBuffer;

/** Luma播放器核心引擎，该类集中管理媒体状态、解码器、AB语义和工作线程，控制命令以独立Task进入同一工作线程
*/
class PlayerEngine
{
public:
	/** 构造未初始化的播放引擎
	*/
	PlayerEngine();

	/** 析构函数会调用uninit，确保工作线程先退出再释放解码资源
	*/
	~PlayerEngine();

private:
	/** 禁止拷贝，播放引擎持有唯一工作线程和解码器
	@param [in] other 另一个对象，不转移其资源所有权
	*/
	PlayerEngine(const PlayerEngine& other);

	/** 禁止赋值，播放引擎持有唯一工作线程和解码器
	@param [in] other 另一个播放引擎
	@return 当前对象
	*/
	PlayerEngine& operator=(const PlayerEngine& other);

public:
	/** 初始化工作线程
	@return 返回初始化结果
	@param [in] config 本次初始化的配置，调用时复制到当前实例
	*/
	LumaPlayerCoreResult init(const LumaPlayerCoreConfig& config);

    /** 提交带真实完成通知的请求，不等待执行
    @param [in] request 请求参数，复制保存
    @param [in] callback 结果线程通知，必须快速返回
    @return Success表示已接受且之后恰有一次完成回报，其他值无回报
    */
    LumaPlayerCoreResult submitAsyncEx(const LumaPlayerCoreRequest& request,
        const LumaPlayerCoreCompletionCallback& callback);

	/** 停止工作线程并释放所有媒体资源
	*/
	void uninit();

	/** 判断引擎是否已经完成初始化
	@return true表示可接收播放命令
	*/
	bool isInit() const;

	/** 设置音频渲染器
	@param [in] audioRender 外部音频渲染器，Core不负责释放
	*/
	void setAudioRender(IAudioRender* audioRender);

	/** 设置视频渲染器
	@param [in] videoRender 外部视频渲染器，Core不负责释放
	*/
	void setVideoRender(IVideoRender* videoRender);

	/** 打开媒体文件，同步等待打开结果
	@param [in] filePath UTF-8路径
	@return 返回打开结果
	*/
	LumaPlayerCoreResult openMedia(const std::string& filePath);

	/** 异步打开媒体文件，只投递命令，不在调用线程等待FFmpeg打开和首帧解码
	@param [in] filePath UTF-8路径
	@return 返回命令投递结果；最终结果通过snapshot中的状态判断
	*/
	LumaPlayerCoreResult openMediaAsync(const std::string& filePath);

	/** 关闭当前媒体文件，同步等待资源释放
	*/
	void closeMedia();

	/** 开始或继续播放
	@return 返回操作结果
	*/
	LumaPlayerCoreResult play();

	/** 异步开始或继续播放，只投递命令不等待
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult playAsync();

	/** 暂停播放
	@return 返回操作结果
	*/
	LumaPlayerCoreResult pause();

	/** 异步暂停播放，只投递命令不等待
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult pauseAsync();

	/** 停止播放并回到起点
	@return 返回操作结果
	*/
	LumaPlayerCoreResult stop();

	/** 跳转到指定位置
	@param [in] position100ns 目标位置，单位100纳秒
	@param [in] keepPlayState true表示定位后播放，false表示定位后暂停
	@return 返回操作结果
	*/
	LumaPlayerCoreResult seekTo(int64_t position100ns, bool keepPlayState);

	/** 异步跳转到指定位置，只投递命令不等待FFmpeg seek完成
	@param [in] position100ns 目标位置，单位100纳秒
	@param [in] keepPlayState true表示定位后播放，false表示定位后暂停
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult seekToAsync(int64_t position100ns, bool keepPlayState);

	/** 只解码并显示指定位置附近的视频帧，不改变原始播放意图
	@param [in] position100ns 目标位置，单位100纳秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult previewFrame(int64_t position100ns);

	/** 异步预览指定位置附近的视频帧，用于进度条拖动等高频交互
	@param [in] position100ns 目标位置，单位100纳秒
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult previewFrameAsync(int64_t position100ns);

	/** 用当前显示帧设置A点
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setLoopAByCurrentFrame();

	/** 按指定媒体位置所在的实际显示帧设置A点
	@param [in] position100ns 进度条对应的媒体位置，单位100纳秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setLoopAAtPosition(int64_t position100ns);

	/** 异步按指定媒体位置设置A点，只投递命令不等待解码定位完成
	@param [in] position100ns 进度条对应的媒体位置，单位100纳秒
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult setLoopAAtPositionAsync(int64_t position100ns);

	/** 用当前显示帧设置B点；如果A点未设置，A点自动设为0秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setLoopBByCurrentFrame();

	/** 按指定媒体位置所在的实际显示帧设置B点；如果A点未设置，A点自动设为0秒
	@param [in] position100ns 进度条对应的媒体位置，单位100纳秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setLoopBAtPosition(int64_t position100ns);

	/** 异步按指定媒体位置设置B点，只投递命令不等待解码定位完成
	@param [in] position100ns 进度条对应的媒体位置，单位100纳秒
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult setLoopBAtPositionAsync(int64_t position100ns);

	/** 清空AB点
	@return 返回操作结果
	*/
	LumaPlayerCoreResult clearLoop();

	/** 异步清空AB点，只投递命令不等待工作线程
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult clearLoopAsync();

	/** 单帧移动A点或B点，先暂停并将画面与播放位置定位到移动后的帧
	@param [in] pointType 要移动的循环点
	@param [in] frameOffset 移动帧数，负数向左，正数向右
	@return 返回操作结果
	*/
	LumaPlayerCoreResult moveLoopPoint(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset);

	/** 异步移动A点或B点，执行时先暂停并同步画面和播放位置
	@param [in] pointType 要移动的循环点
	@param [in] frameOffset 移动帧数，负数向左，正数向右
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult moveLoopPointAsync(LumaPlayerCoreLoopPointType pointType, int32_t frameOffset);

	/** 设置播放倍速
	@param [in] ratePermille 播放倍速，1000表示1.0倍速
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setPlaybackRatePermille(int32_t ratePermille);

	/** 异步设置播放倍速，只投递命令不等待工作线程
	@param [in] ratePermille 播放倍速，1000表示1.0倍速
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult setPlaybackRatePermilleAsync(int32_t ratePermille);

	/** 获取当前快照
	@return 返回线程安全复制出的播放快照
	*/
	LumaPlayerSnapshot snapshot() const;

	/** 获取当前媒体信息
	@return 返回线程安全复制出的媒体信息
	*/
	LumaPlayerMediaInfo mediaInfo() const;

	/** 获取最后一次错误文本
	@return 返回UTF-8错误文本
	*/
	std::string lastError() const;

	/** 工作任务入口，只允许PlayerWorkerTask调用
	@param [in] task 当前工作任务
    @param [in] exitFlag 任务存活期间借用的退出标记，可空
	*/
    void workerLoop(PlayerWorkerTask* task, const std::atomic<bool>* exitFlag);

	/** 预览任务入口，只允许预览Task调用；该函数不推进正式播放时钟
	@param [in] position100ns 需要预览的媒体时间，单位100纳秒
	@param [in] requestSerial 本次预览序号，用于丢弃过期预览
	@param [in] exitFlag 预览Task的退出标记，可以为空
	@return 返回预览执行结果
	*/
	LumaPlayerCoreResult previewFrameFromTask(int64_t position100ns, uint64_t requestSerial, const std::atomic<bool>* exitFlag);

public:
    /** 执行控制任务并根据最新状态恢复播放调度
    @param [in] command 待执行命令
    @param [in] exitFlag 当前任务退出标记，任务执行期间有效
    */
    void executeCommandTask(const PlayerCommand& command, const std::atomic<bool>* exitFlag);

    /** 取消尚未完成的端点移动，不等待解码退出
    */
    void cancelLoopPointMove();

private:
    /** 根据最新播放状态安排低优先级播放任务
    */
    void resumePlaybackTask();

	/** 投递同步命令并等待工作线程返回
	@param [in,out] command 命令内容
	@param [in] timeoutMs 等待超时时间
	@return 返回命令执行结果
	*/
	LumaPlayerCoreResult postSyncCommand(PlayerCommand& command, int32_t timeoutMs);

	/** 投递异步命令并唤醒工作线程
	@param [in] command 命令内容
	@return 成功返回Success，否则返回具体的提交或输出错误码
	*/
	LumaPlayerCoreResult postCommand(const PlayerCommand& command);

	/** 投递异步预览任务到独立预览线程
	@param [in] position100ns 目标位置，单位100纳秒
	@param [in] requestSerial 本次预览序号
	@return 成功返回Success，否则返回具体的提交或输出错误码
	*/
	LumaPlayerCoreResult postPreviewTask(int64_t position100ns, uint64_t requestSerial);

	/** 递增预览请求序号，使正在执行或排队的旧预览命令失效
	@return 返回新的预览请求序号，可写入需要保序的命令
	*/
	uint64_t nextPreviewSerial();

	/** 完成同步命令并唤醒等待线程
	@param [in] command 已执行命令
	@param [in] result 执行结果
	*/
	void completeSyncCommand(const PlayerCommand& command, LumaPlayerCoreResult result);

	/** 在工作线程处理一条命令
	@param [in] command 命令内容
	@param [in] exitFlag 当前任务取消标记，内部同步调用可为空
	*/
	void handleCommand(const PlayerCommand& command, const std::atomic<bool>* exitFlag = nullptr);

	/** 打开媒体文件的工作线程实现
	@param [in] filePath UTF-8路径
    @param [in] exitFlag 可空，单次打开返回后在安全阶段取消
	@return 返回打开结果
	*/
    LumaPlayerCoreResult openMediaInternal(const std::string& filePath, const std::atomic<bool>* exitFlag = nullptr);

	/** 关闭媒体文件的工作线程实现
	*/
	void closeMediaInternal();

	/** 工作线程播放推进一步
	@param [in] task 当前工作任务
    @param [in] exitFlag 任务存活期间借用的退出标记，可空
	*/
    void playbackStep(PlayerWorkerTask* task, const std::atomic<bool>* exitFlag);

	/** 跳转到指定位置并可选恢复播放
	@param [in] position100ns 目标位置，单位100纳秒
	@param [in] resumeAfterSeek true表示seek后继续播放
	@param [in] renderPreview true表示seek后同步刷新目标位置画面
    @param [in] exitFlag 可空，定位到一致状态后检查取消
	@return 返回seek结果
	*/
    LumaPlayerCoreResult seekInternal(int64_t position100ns, bool resumeAfterSeek, bool renderPreview,
        const std::atomic<bool>* exitFlag = nullptr);

	/** 使用预览reader重新读取循环点所在显示帧，保证B点右边界来自下一帧真实时间戳
	@param [in,out] pointInfo 需要修正的循环点
	@param [in] position100ns 设置循环点时的媒体位置，单位100纳秒
	@return true表示修正成功，false表示保留原循环点
	*/
    bool refineLoopPointByPreview(LumaPlayerLoopPointInfo* pointInfo, int64_t position100ns,
        const std::atomic<bool>* exitFlag);

	/** 解码并显示指定位置附近的一帧
	@param [in] position100ns 目标位置，单位100纳秒
	@param [in] requestSerial 预览请求序号；0表示内部同步预览，不参与拖动取消
	@param [in] exitFlag 当前Task退出标记，可以为空
	@param [in] updatePlaybackPosition true表示同步正式播放位置和时钟
	@return 返回预览结果
	*/
	LumaPlayerCoreResult previewFrameInternal(int64_t position100ns, uint64_t requestSerial, const std::atomic<bool>* exitFlag, bool updatePlaybackPosition);

	/** 从当前循环点查找相邻视频帧，不改变正式播放reader的位置
	@param [in] origin 当前循环点
	@param [in] direction 小于0表示上一帧，大于0表示下一帧
	@param [out] frame 相邻视频帧
	@param [in] requestSerial 本次预览代次
	@param [in] exitFlag 当前任务退出标记，可为空
	@return 返回查找结果
	*/
	LumaPlayerCoreResult readAdjacentVideoFrame(const LumaPlayerLoopPointInfo& origin, int32_t direction, LumaPlayerVideoFrame* frame, uint64_t requestSerial, const std::atomic<bool>* exitFlag);

	/** 读取下一帧视频
	@param [out] frame 视频帧
	@return 返回读取结果
	*/
	LumaPlayerCoreResult readNextVideoFrame(LumaPlayerVideoFrame* frame);

	/** 读取下一帧音频
	@param [out] frame 音频帧
	@return 返回读取结果，Ended用StateError表示本轮读取到结尾
	*/
	LumaPlayerCoreResult readNextAudioFrame(LumaPlayerAudioFrame* frame);

	/** 渲染视频帧并更新当前显示帧快照
	@param [in] frame 视频帧
	@param [in] updatePlaybackPosition true表示该帧来自正式播放或同步seek，需要更新播放位置
	@return 成功返回Success，否则返回具体的输出错误码
	*/
	LumaPlayerCoreResult renderVideoFrame(const LumaPlayerVideoFrame& frame, bool updatePlaybackPosition);

	/** 渲染音频帧
	@param [in] frame 音频帧
	@return 成功返回Success，否则返回具体的输出错误码
	*/
	LumaPlayerCoreResult renderAudioFrame(const LumaPlayerAudioFrame& frame);

	/** 裁剪音频到当前播放片段，避免AB右边界外的声音被送出
	@param [in] source 原始音频帧
	@param [in] start100ns 允许播放的起点
	@param [in] end100ns 允许播放的右边界
	@param [out] clipped 裁剪后的音频帧
	@return true表示裁剪后仍有可播放PCM
	*/
	bool clipAudioFrame(const LumaPlayerAudioFrame& source, int64_t start100ns, int64_t end100ns, LumaPlayerAudioFrame* clipped) const;

	/** 跳回当前循环起点，AB循环优先，否则回到文件起点
	*/
	void replayFromLoopStart();

    /** 提前准备下一轮循环，端点或媒体变化时取消旧任务
    */
    void refreshLoopBuffer();

    /** 释放循环缓存并取消尚未完成的预备任务
    */
    void clearLoopBuffers();

    /** 读取并呈现本轮真正的首帧，准备好后才允许时钟推进
    @param [in] position100ns 本轮起点，单位100纳秒
    @return 首帧准备结果
    */
    LumaPlayerCoreResult primePlayback(int64_t position100ns);

	/** 更新共享快照中的状态和位置
	@param [in] state 新状态
	@param [in] position100ns 新播放位置
	*/
	void updateSnapshot(LumaPlayerCoreState state, int64_t position100ns);

	/** 设置最后一次错误
	@param [in] result 具体操作结果，中文描述由统一映射提供
	*/
	void setLastError(LumaPlayerCoreResult result);

	/** 将输入位置裁剪到媒体时长范围
	@param [in] position100ns 原始位置
	@return 返回裁剪后的位置
	*/
	int64_t clampPosition(int64_t position100ns) const;

	/** 获取当前有效播放片段起点
	@return AB有效时返回A点，否则返回0
	*/
	int64_t activeSegmentStart100ns() const;

	/** 获取当前有效播放片段右边界
	@return AB有效时返回B点，否则返回媒体总时长
	*/
	int64_t activeSegmentEnd100ns() const;

	/** 判断读取结果是否表示流结束
	@param [in] result 读取结果
	@return true表示当前流读到结尾
	*/
	bool isReadEnd(LumaPlayerCoreResult result) const;

private:
	// 生命周期和共享快照锁
	mutable std::mutex m_mutex;
	// init复制的运行参数，只在初始化完成后读取
	LumaPlayerCoreConfig m_config;
	// 串行化命令投递和取消
	std::mutex m_submitMutex;
	// 工作线程任务对象，生命周期由Core持有并由CTaskThreadManager执行
	std::shared_ptr<PlayerWorkerTask> m_workerTask;
	// 常驻预览任务，拖动只覆盖待处理位置
	std::shared_ptr<PlayerPreviewTask> m_previewTask;
	// CTaskThreadManager生成的工作线程ID
    // 可选异步提交与关闭入口互斥，不在回调期间持有
    std::mutex m_asyncSubmitMutex;
	uint32_t m_threadId;
    // 完成通知管理器线程ID，生产者退出后最后回收
    uint32_t m_completionThreadId;
    // 独立结果队列
    std::shared_ptr<PlayerCompletionTask> m_completionTask;
	// CTaskThreadManager生成的预览线程ID，高频拖动预览独立排队
	uint32_t m_previewThreadId;
	// 独立循环预备线程，不阻塞交互预览
	uint32_t m_loopThreadId;
	// 当前是否已经完成初始化
	bool m_isInit;
	// 预览线程身份，用于拒绝回调内同步等待
	std::thread::id m_previewStdThreadId;
	// 工作线程的std::thread id，用于避免工作线程同步等待自己
	std::thread::id m_workerStdThreadId;
	// 外部音频渲染器，Core不负责释放
	IAudioRender* m_audioRender;
	// 外部视频渲染器，Core不负责释放
	IVideoRender* m_videoRender;
	// 当前播放时钟，仅在工作线程读写
	MediaClock m_clock;
	// 当前播放状态，仅工作线程直接修改，快照通过m_mutex同步给外部
	LumaPlayerCoreState m_state;
	// 成功打开媒体后递增的序号，仅工作线程修改
	uint64_t m_mediaSerial;
	// 当前媒体信息
	LumaPlayerMediaInfo m_mediaInfo;
	// 当前AB循环范围
	LumaPlayerLoopRange m_loopRange;
	// 最近一次显示的视频帧边界
	LumaPlayerLoopPointInfo m_currentVideoFrame;
	// 当前播放位置，单位100纳秒
	int64_t m_position100ns;
	// 播放倍速，1000表示1.0倍速
	int32_t m_ratePermille;
	// 是否文件结束后自动重播
	bool m_enableAutoReplay;
    // 当前播放路径是否在B点循环；手动跳出B后禁用，跳回B前重新启用
    bool m_loopBoundaryEnabled;
	// 共享给外部的播放快照
	LumaPlayerSnapshot m_snapshot;
	// 最后一次错误文本
	std::string m_lastError;
	// 视频解码reader，只在工作线程使用
	FFmpegCppPlaybackReader* m_videoReader;
	// 音频解码reader，只在工作线程使用
	FFmpegCppPlaybackReader* m_audioReader;
	// 保护预览reader，播放主线程和预览任务线程都会读取它
	mutable std::mutex m_previewReaderMutex;
	// 预览reader，只能在m_previewReaderMutex保护下使用
	FFmpegCppPlaybackReader* m_previewReader;
	// 下一帧待显示视频
	LumaPlayerVideoFrame m_pendingVideoFrame;
	// 下一段待播放音频
	LumaPlayerAudioFrame m_pendingAudioFrame;
	// 下一帧视频是否有效
	bool m_hasPendingVideoFrame;
	// 下一段音频是否有效
	bool m_hasPendingAudioFrame;
	// 视频流是否已经读到结尾
	bool m_videoEnded;
	// 音频流是否已经读到结尾
	bool m_audioEnded;
	// 当前视频帧索引计数，FFmpegCpp未提供时由Core递增
	int64_t m_videoFrameIndex;
	// seek后需要跳过的解码下限，避免显示目标位置之前已经结束的帧
	int64_t m_decodeFloor100ns;
	// 拖动预览序号，新的预览命令会让旧命令失效
	std::atomic<uint64_t> m_previewSerial;
    // 下一轮后台准备任务，播放线程持有
    std::shared_ptr<PlayerLoopBuffer> m_preparedLoopBuffer;
    // 当前轮使用的只读起始缓存
    std::shared_ptr<PlayerLoopBuffer> m_activeLoopBuffer;
    // 当前轮已读取的视频缓存下标
    size_t m_loopVideoIndex;
    // 当前轮已读取的音频缓存下标
    size_t m_loopAudioIndex;
};