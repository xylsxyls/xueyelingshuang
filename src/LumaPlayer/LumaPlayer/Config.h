#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"
#include <atomic>
#include <stdint.h>

/** 播放器进程配置，main最先初始化，所有窗口共享默认参数
*/
class Config
{
public:
	/** 获取进程配置实例
	@return 配置单例引用
	*/
	static Config& instance();

	/** 解析启动参数并初始化默认Core配置
	@param [in] argc 参数个数
	@param [in] argv 命令行参数
	*/
	void init(int argc, char* argv[]);

	/** 清理配置运行状态
	*/
	void uninit();

private:
	/** 构造配置默认值
	*/
	Config();

	/** 禁止复制进程配置
	@param [in] other 其他配置对象
	*/
	Config(const Config& other);

	/** 禁止覆盖配置实例
	@param [in] other 其他配置对象
	@return 当前对象引用
	*/
	Config& operator=(const Config& other);

public:
	// 顶部悬浮条高度，单位像素
	int32_t m_topOverlayHeight;
	// 底部悬浮条高度，单位像素
	int32_t m_bottomOverlayHeight;
	// 底部播放按钮边长，单位像素
	int32_t m_playButtonSize;
	// 进度轨道到下浮框顶部的距离，单位像素
	int32_t m_progressTrackTop;
	// 悬浮条单次移动距离，单位像素
	int32_t m_overlayStep;
	// 鼠标静止隐藏延时，单位毫秒
	int32_t m_mouseIdleHideMs;
	// 窗口边缘缩放判定宽度，单位像素
	int32_t m_resizeMargin;
	// 进度条触发边距，单位像素
	int32_t m_progressTriggerMargin;
	// 顶部悬浮触发边距，单位像素
	int32_t m_topTriggerMargin;
	// 最小窗口宽度，单位像素
	int32_t m_minWindowWidth;
	// 最小窗口高度，单位像素
	int32_t m_minWindowHeight;
	// 最小缩放百分比
	int32_t m_minZoomPercent;
	// 最大缩放百分比
	int32_t m_maxZoomPercent;
	// 拖动预览提交间隔，单位毫秒
	int64_t m_previewDragIntervalMs;
	// 音频设备专用线程的服务间隔，单位毫秒
	int32_t m_audioServiceIntervalMs;
	// 每秒对应的100纳秒刻度数
	int64_t m_second100ns;
	// 每毫秒对应的100纳秒刻度数
	int64_t m_millisecond100ns;
	// 音频排队上限，单位100纳秒
	int64_t m_audioMaxQueue100ns;
	// 命令行是否包含debug诊断标记，基础日志始终开启
	bool m_debugEnabled;
	// 日志服务是否已经启动，跨线程读取
	std::atomic<bool> m_logInitialized;
	// 注入Core的独立实例配置
	LumaPlayerCoreCConfig m_coreConfig;
};

#define g_config Config::instance()