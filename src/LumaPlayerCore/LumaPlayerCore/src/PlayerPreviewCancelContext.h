#pragma once
#include <atomic>
#include <stdint.h>

/** 预览取消参数，借用请求序号和任务退出标记，相关对象必须覆盖解码调用的生命周期
*/
struct PlayerPreviewCancelContext
{
public:
	// 最新预览序号，生命周期由PlayerEngine保证覆盖本次调用
	const std::atomic<uint64_t>* m_previewSerial;
	// 预览Task退出标记，生命周期由当前Task保证覆盖本次调用
	const std::atomic<bool>* m_exitFlag;
	// 当前预览命令自己的序号
	uint64_t m_requestSerial;

public:
	/** 构造空上下文，默认不取消
	*/
	PlayerPreviewCancelContext();
};