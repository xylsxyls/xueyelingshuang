#pragma once
#include "LumaPlayerCoreRequest.h"
#include "LumaPlayerCoreConfig.h"
#include "IAudioRender.h"
#include "IVideoRender.h"
#include "LumaPlayerCoreMacro.h"
#include "LumaPlayerCoreTypes.h"
#include <atomic>

class PlayerEngine;

/** 流光播放器核心入口，外部通过该类打开媒体、控制播放、设置AB循环和注入音视频渲染器；Core不依赖Qt或平台窗口类型
*/
class LumaPlayerCoreAPI LumaPlayerCore
{
public:
    /** 提交可选异步请求并通过专用结果线程回报真实完成
    @param [in] request 参数和宿主请求身份，调用时复制
    @param [in] callback 完成通知，禁止阻塞或在其中销毁Core
    @return Success表示接受，其余为提交失败且不回调
    */
    LumaPlayerCoreResult submitAsyncEx(const LumaPlayerCoreRequest& request,
        const LumaPlayerCoreCompletionCallback& callback);

	/** 构造未初始化的播放器核心
	*/
	LumaPlayerCore();

	/** 析构函数会调用uninit，确保内部线程和解码资源释放
	*/
	~LumaPlayerCore();

private:
	/** 禁止拷贝，Core内部持有唯一播放引擎
	@param [in] other 另一个对象，不转移其资源所有权
	*/
	LumaPlayerCore(const LumaPlayerCore& other);

	/** 禁止赋值，Core内部持有唯一播放引擎
	@param [in] other 另一个Core对象
	@return 当前对象
	*/
	LumaPlayerCore& operator=(const LumaPlayerCore& other);

public:
	/** 初始化Core工作线程
	@return 返回初始化结果
	*/
	LumaPlayerCoreResult init();

	/** 使用外部配置初始化，配置复制到当前实例，必须在播放控制前调用
	@param [in] config 本次初始化使用的配置
	@return 参数无效返回InvalidParam，成功返回Success
	*/
	LumaPlayerCoreResult init(const LumaPlayerCoreConfig& config);

	/** 释放Core工作线程和当前媒体资源
	*/
	void uninit();

	/** 判断Core是否已经完成初始化
	@return true表示可以接收播放控制命令
	*/
	bool isInit() const;

	/** 设置是否输出Core日志
	@param [in] enabled true表示允许业务Core使用LogManager记录操作结果，默认false
	*/
	void setLogEnabled(bool enabled);

	/** 设置音频渲染器
	@param [in] audioRender 外部创建的渲染器，Core只保存指针不负责释放；仅在init前或uninit后设置
	*/
	void setAudioRender(IAudioRender* audioRender);

	/** 设置视频渲染器
	@param [in] videoRender 外部创建的渲染器，Core只保存指针不负责释放；仅在init前或uninit后设置
	*/
	void setVideoRender(IVideoRender* videoRender);

	/** 打开媒体文件
	@param [in] filePath UTF-8文件路径
	@return 返回打开结果
	*/
	LumaPlayerCoreResult openMedia(const std::string& filePath);

	/** 异步打开媒体文件，调用线程不会等待FFmpeg打开和首帧解码
	@param [in] filePath UTF-8文件路径
	@return 返回命令投递结果；最终结果通过snapshot中的状态判断
	*/
	LumaPlayerCoreResult openMediaAsync(const std::string& filePath);

	/** 关闭当前媒体文件
	*/
	void closeMedia();

	/** 开始或继续播放
	@return 返回操作结果
	*/
	LumaPlayerCoreResult play();

	/** 异步开始或继续播放，调用线程不会等待Core完成
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult playAsync();

	/** 暂停播放
	@return 返回操作结果
	*/
	LumaPlayerCoreResult pause();

	/** 异步暂停播放，调用线程不会等待Core完成
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

	/** 异步跳转到指定位置，高频拖动时不会阻塞调用线程
	@param [in] position100ns 目标位置，单位100纳秒
	@param [in] keepPlayState true表示seek后恢复播放
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult seekToAsync(int64_t position100ns, bool keepPlayState);

	/** 只预览指定位置附近的视频帧
	@param [in] position100ns 目标位置，单位100纳秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult previewFrame(int64_t position100ns);

	/** 异步预览指定位置附近的视频帧
	@param [in] position100ns 目标位置，单位100纳秒
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult previewFrameAsync(int64_t position100ns);

	/** 按当前显示帧设置A点
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

	/** 按当前显示帧设置B点；如果A点未设置，A点自动取0秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setLoopBByCurrentFrame();

	/** 按指定媒体位置所在的实际显示帧设置B点；如果A点未设置，A点自动取0秒
	@param [in] position100ns 进度条对应的媒体位置，单位100纳秒
	@return 返回操作结果
	*/
	LumaPlayerCoreResult setLoopBAtPosition(int64_t position100ns);

	/** 异步按指定媒体位置设置B点，只投递命令不等待解码定位完成
	@param [in] position100ns 进度条对应的媒体位置，单位100纳秒
	@return 返回命令投递结果
	*/
	LumaPlayerCoreResult setLoopBAtPositionAsync(int64_t position100ns);

	/** 清空AB循环点
	@return 返回操作结果
	*/
	LumaPlayerCoreResult clearLoop();

	/** 异步清空AB循环点，只投递命令不等待工作线程
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

	/** 取消未完成的AB端点移动，不等待解码退出
	*/
	void cancelLoopPointMove();

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

	/** 获取当前播放快照
	@return 返回播放快照
	*/
	LumaPlayerSnapshot snapshot() const;

	/** 获取当前媒体信息
	@return 返回媒体信息
	*/
	LumaPlayerMediaInfo mediaInfo() const;

	/** 获取最后一次错误
	@return 返回UTF-8错误文本
	*/
	std::string lastError() const;

private:

private:
	// Core内部播放引擎，LumaPlayerCore负责创建和释放
	PlayerEngine* m_engine;
	// 业务日志开关，默认关闭
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::atomic<bool> m_logEnabled;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};