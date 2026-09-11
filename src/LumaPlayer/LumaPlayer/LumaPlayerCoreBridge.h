#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"

#include <stdint.h>
#include <string>

class LumaPlayerAudioRender;
class LumaPlayerVideoRender;

/** Qt播放器到Core C接口的桥接层，隔离不同运行库之间的C++ ABI
*/
class LumaPlayerCoreBridge
{
public:
    /** 提交携带身份和真实完成回报的C请求
    @param [in] request 参数副本，字符串仅提交期间借用
    @param [in] callback 短小结果回调
    @param [in] userData 借用上下文，存活到uninit结束
    @return 提交结果，非零时不回调
    */
    int32_t submitAsyncEx(const LumaPlayerCoreCRequest& request,
        LumaPlayerCoreCCompletionCallback callback, void* userData);

	/** 创建Core句柄并记录外部渲染器
	@param [in] audioRender Qt音频渲染器，由窗口管理生命周期
	@param [in] videoRender Qt视频渲染器，由窗口管理生命周期
	*/
	LumaPlayerCoreBridge(LumaPlayerAudioRender* audioRender, LumaPlayerVideoRender* videoRender);

	/** 停止Core线程并销毁由Core DLL创建的句柄
	*/
	~LumaPlayerCoreBridge();

	/** 注册渲染回调、开启业务日志并注入配置初始化Core
	@param [in] debugEnabled 保留的debug诊断标记，不控制基础日志开关
	@return 返回LumaPlayerCoreCResult数值
	*/
	int32_t init(bool debugEnabled);

	/** 停止Core线程和媒体处理，重复调用安全
	*/
	void uninit();

	/** 获取供界面读取的播放快照
	@return 返回当前快照；句柄不可用时返回全零快照
	*/
	LumaPlayerCoreCSnapshot snapshot() const;

	/** 获取Core最后一次错误文本
	@return 返回UTF-8文本，过长内容会被截断到本地诊断缓冲区
	*/
	std::string lastError() const;

	/** 异步打开UTF-8媒体路径，返回命令投递结果
	@param [in] filePath 调用所需的filePath参数
	*/
	int32_t openMediaAsync(const std::string& filePath);

	/** 异步开始或恢复播放，返回命令投递结果
	*/
	int32_t playAsync();

	/** 异步暂停播放，返回命令投递结果
	*/
	int32_t pauseAsync();

	/** 异步跳转到目标时间，keepPlayState决定完成后是否播放
	@param [in] position100ns 调用所需的position100ns参数
	@param [in] keepPlayState 调用所需的keepPlayState参数
	*/
	int32_t seekToAsync(int64_t position100ns, bool keepPlayState);

	/** 异步解码目标位置预览帧，不改变正式播放时钟
	@param [in] position100ns 调用所需的position100ns参数
	*/
	int32_t previewFrameAsync(int64_t position100ns);

	/** 按目标位置实际显示帧异步设置A点
	@param [in] position100ns 调用所需的position100ns参数
	*/
	int32_t setLoopAAtPositionAsync(int64_t position100ns);

	/** 按目标位置实际显示帧异步设置B点
	@param [in] position100ns 调用所需的position100ns参数
	*/
	int32_t setLoopBAtPositionAsync(int64_t position100ns);

	/** 异步取消AB循环
	*/
	int32_t clearLoopAsync();

	/** 异步按帧移动循环点；pointType为0表示A点，1表示B点
	@param [in] pointType 调用所需的pointType参数
	@param [in] frameOffset 调用所需的frameOffset参数
	*/
	int32_t moveLoopPointAsync(int32_t pointType, int32_t frameOffset);

	/** 取消未完成的AB移动
	*/
	void cancelLoopPointMove();

	/** 异步设置千分比倍速，1000表示1.0倍
	@param [in] ratePermille 调用所需的ratePermille参数
	*/
	int32_t setPlaybackRatePermilleAsync(int32_t ratePermille);

private:
	/** 禁止复制拥有Core句柄的桥接器
	@param [in] other 另一桥接器
	*/
	LumaPlayerCoreBridge(const LumaPlayerCoreBridge& other) = delete;

	/** 禁止复制Core句柄所有权
	@param [in] other 另一桥接器
	@return 当前对象
	*/
	LumaPlayerCoreBridge& operator=(const LumaPlayerCoreBridge& other) = delete;

	/** 把Core的音频打开请求转发到Qt音频渲染器
	@param [in] format 外部输出设备需要的格式
	@param [in] userData 调用所需的userData参数
	*/
	static int32_t openAudioCallback(const LumaPlayerCoreCAudioFormat* format, void* userData);

	/** 把Core的音频关闭请求转发到Qt音频渲染器
	@param [in] userData 调用所需的userData参数
	*/
	static void closeAudioCallback(void* userData);

	/** 把Core的音频清空请求转发到Qt音频渲染器
	@param [in] userData 调用所需的userData参数
	*/
	static void flushAudioCallback(void* userData);

	/** 把Core的音频暂停请求转发到Qt音频渲染器
	@param [in] userData 调用所需的userData参数
	*/
	static void pauseAudioCallback(void* userData);

	/** 把Core的音频恢复请求转发到Qt音频渲染器
	@param [in] userData 调用所需的userData参数
	*/
	static void resumeAudioCallback(void* userData);

	/** 在回调有效期内把PCM复制到Qt音频渲染器
	@param [in] frame 回调期间借用的媒体帧
	@param [in] userData 调用所需的userData参数
	*/
	static int32_t renderAudioCallback(const LumaPlayerCoreCAudioFrame* frame, void* userData);

	/** 查询Qt侧尚未消费的PCM时长
	@param [in] userData 调用所需的userData参数
	*/
	static int64_t bufferedAudioCallback(void* userData);

	/** 把Core的视频打开请求转发到Qt视频渲染器
	@param [in] format 外部输出设备需要的格式
	@param [in] userData 调用所需的userData参数
	*/
	static int32_t openVideoCallback(const LumaPlayerCoreCVideoFormat* format, void* userData);

	/** 把Core的视频关闭请求转发到Qt视频渲染器
	@param [in] userData 调用所需的userData参数
	*/
	static void closeVideoCallback(void* userData);

	/** 在回调有效期内把BGRA帧复制到Qt视频渲染器
	@param [in] frame 回调期间借用的媒体帧
	@param [in] userData 调用所需的userData参数
	*/
	static int32_t renderVideoCallback(const LumaPlayerCoreCVideoFrame* frame, void* userData);

private:
	// Core DLL创建并销毁的不透明句柄
	LumaPlayerCoreHandle m_handle;
	// 外部拥有的Qt音频渲染器，Core停止前必须保持有效
	LumaPlayerAudioRender* m_audioRender;
	// 外部拥有的Qt视频渲染器，Core停止前必须保持有效
	LumaPlayerVideoRender* m_videoRender;
	// 防止重复执行Core释放流程
	bool m_initialized;
};