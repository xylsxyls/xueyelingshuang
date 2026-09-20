#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"

#include <stdint.h>
#include <memory>
#include <atomic>

#include <QByteArray>
#include <QMutex>

class LumaPlayerAudioTask;
class QAudioOutput;
class QIODevice;

/** Qt桌面端音频渲染器，Core线程只入队PCM，音频服务线程定时写入QAudioOutput
*/
class LumaPlayerAudioRender
{
public:
	/** 构造一个未打开音频设备的渲染器
	*/
	LumaPlayerAudioRender();

	/** 析构时释放Qt音频设备，调用方应先停止Core线程
	*/
	~LumaPlayerAudioRender();

	/** 启动独立设备任务，必须在Core启动前调用
	@param [in] intervalMs 服务间隔，单位毫秒，必须大于0
	@return 成功返回0，失败返回Core结果枚举对应数值
	*/
	int32_t init(int32_t intervalMs);

	/** 停止设备任务并等待设备释放，调用前必须停止Core回调
	*/
	void uninit();

	/** 接收Core的PCM格式并请求音频服务线程重新打开音频设备
	@param [in] format 外部输出设备需要的格式
	*/
	bool openAudio(const LumaPlayerCoreCAudioFormat& format);

	/** 清空PCM并请求音频服务线程关闭音频设备
	*/
	void closeAudio();

	/** 丢弃时间轴跳转前的PCM并请求音频服务线程重置设备
	*/
	void flushAudio();

	/** 请求音频服务线程暂停音频设备并保留缓存
	*/
	void pauseAudio();

	/** 请求音频服务线程恢复音频设备
	*/
	void resumeAudio();

	/** 在Core回调返回前复制一块PCM到Qt队列
	@param [in] frame 回调期间借用的媒体帧
	*/
	bool renderAudio(const LumaPlayerCoreCAudioFrame& frame);

	/** 获取尚未写入音频设备的PCM时长，单位100纳秒
	*/
	int64_t bufferedDuration100ns() const;

	/** 音频服务线程定时调用，把Core线程积累的PCM写入Qt音频输出
	*/
	void processAudioQueue();

	/** 音频服务线程调用，立即关闭Qt音频输出
	*/
	void shutdownInAudioThread();

    /** 设置本实例输出增益，不访问Qt设备，不修改系统音量
    @param [in] percent 百分比，范围0至配置的最大视频缩放百分比
    @return 是否接受合法目标
    */
    bool setVolumePercent(int32_t percent);

    /** 对完整16位小端PCM样本执行饱和增益，不改变数据长度
    @param [in,out] data 独占待输出副本，长度必须为偶数
    @param [in] percent 已校验的非负音量百分比
    */
    static void applyVolume(QByteArray& data, int32_t percent);

private:
	/** 根据当前格式估算缓存字节数对应的播放时长
	@param [in] byteCount 调用所需的byteCount参数
	*/
	int64_t bytesToDuration100ns(size_t byteCount) const;

	/** 获取当前PCM格式中一个采样帧占用的字节数，无效格式返回0
	*/
	int32_t bytesPerFrame() const;

	/** 判断当前PCM格式能否交给Qt音频设备
	*/
	bool isFormatValid() const;

	/** 限制缓存长度，避免音频设备异常时无限增长
	*/
	void trimQueueNoLock();

private:
	// 设备任务线程ID，仅初始化和反初始化线程访问
	uint32_t m_threadId;
	// 常驻音频任务，在等待结束前保留所有权
	std::shared_ptr<LumaPlayerAudioTask> m_task;
	// 保护待播放PCM和跨线程标记
	mutable QMutex m_mutex;
	// 音频设备打开或写入失败，下一次PCM提交报告失败
	bool m_deviceFailed;
	// Core最新请求的PCM格式
	LumaPlayerCoreCAudioFormat m_format;
	// 等待写入Qt音频设备的PCM数据
	QByteArray m_pcmQueue;
	// true表示音频服务线程需要重新打开音频设备
	bool m_pendingOpen;
	// true表示音频服务线程需要清空音频设备和队列
	bool m_pendingFlush;
	// true表示音频服务线程需要关闭音频设备
	bool m_pendingClose;
	// Core期望音频设备保持暂停；该状态在音频服务线程应用
	bool m_shouldPauseAudio;
	// true表示音频服务线程需要把最新暂停状态应用到音频设备
	bool m_pendingPlaybackState;
	// PCM队列版本，清空队列时递增，防止并发写入后误删新数据
	uint64_t m_queueGeneration;

	/** 音频设备生命周期序号，防止旧设备错误污染新媒体 */
	uint64_t m_deviceGeneration;
	// 音频服务线程持有的Qt音频输出对象
	QAudioOutput* m_audioOutput;
	// QAudioOutput启动后返回的写入设备
	QIODevice* m_audioDevice;
    // 跨线程最新增益目标，设备重开不重置
    std::atomic<int32_t> m_volumePercent;
    // 设备线程保留已转换块，短写后继续同一字节序列
    QByteArray m_outputData;
    // 当前转换块已写字节数
    int32_t m_outputOffset;
    // 转换块对应原始PCM队列代次
    uint64_t m_outputGeneration;
};