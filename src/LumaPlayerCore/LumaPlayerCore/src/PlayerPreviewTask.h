#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdint.h>

class PlayerEngine;

/** 常驻视频预览任务，只保留最新待处理位置，不改变正式播放读取器的位置
*/
class PlayerPreviewTask : public CTask
{
public:
	// 任务类别，用于取消运行中和排队中的旧预览
	static const int32_t kTaskId = 2001;

	/** 构造预览任务
	@param [in] engine 播放引擎指针，任务不负责释放，必须保证任务执行期间有效
	*/
	explicit PlayerPreviewTask(PlayerEngine* engine);

	/** 析构函数不做耗时清理，预览取消通过StopTask控制
	*/
	virtual ~PlayerPreviewTask();

private:
	/** 禁止拷贝，避免多个Task共享同一退出标记
	@param [in] other 另一个对象，不转移其资源所有权
	*/
	PlayerPreviewTask(const PlayerPreviewTask& other);

	/** 禁止赋值，避免多个Task共享同一退出标记
	@param [in] other 另一个预览任务
	@return 当前对象
	*/
	PlayerPreviewTask& operator=(const PlayerPreviewTask& other);

public:
	/** 覆盖唯一待处理请求并唤醒预览线程
	@param [in] position100ns 预览位置，单位100纳秒
	@param [in] requestSerial 请求序号，旧请求不能覆盖新请求
	*/
	void request(int64_t position100ns, uint64_t requestSerial);

	/** 等待并处理最新预览，解码期间不持有请求锁
	*/
	virtual void DoTask();

	/** 请求预览任务尽快退出，FFmpeg读取过程会通过取消回调观察该标记
	*/
	virtual void StopTask();

private:
	// 播放引擎指针，不负责释放
	PlayerEngine* m_engine;
	// 保护待处理请求
	std::mutex m_requestMutex;
	// 新请求和停止通知
	std::condition_variable m_requestReady;
	// 是否有尚未取出的请求
	bool m_pending;
	// 需要预览的媒体时间，单位100纳秒
	int64_t m_position100ns;
	// 本次预览请求序号
	uint64_t m_requestSerial;
	// 退出标记，StopTask会设置为true
	std::atomic<bool> m_exit;
};