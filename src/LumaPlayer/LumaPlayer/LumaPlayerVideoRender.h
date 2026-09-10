#pragma once
#ifndef LUMAPLAYERCORE_USE_C_API
#define LUMAPLAYERCORE_USE_C_API
#endif
#include "LumaPlayerCore/LumaPlayerCoreAPI.h"

#include <QImage>
#include <QMutex>
#include <QSize>

/** Qt桌面端视频渲染器，接收Core的BGRA帧并缓存给窗口绘制
*/
class LumaPlayerVideoRender
{
public:
	/** 构造一个空视频渲染器
	*/
	LumaPlayerVideoRender();

	/** 记录Core通知的视频尺寸并清空旧帧
	@param [in] format 外部输出设备需要的格式
	*/
	bool openVideo(const LumaPlayerCoreCVideoFormat& format);

	/** 清空当前视频尺寸和图像
	*/
	void closeVideo();

	/** 在Core回调返回前复制一帧BGRA图像
	@param [in] frame 回调期间借用的媒体帧
	*/
	bool renderVideo(const LumaPlayerCoreCVideoFrame& frame);

	/** 拷贝最近一帧图像，供GUI线程绘制
	@param [out] image 输出图像
	@param [out] frameStart100ns 帧起始时间，可以为空
	@param [out] frameEnd100ns 帧结束边界，可以为空
	@param [out] frameSerial 帧更新序号，可以为空
	@return true表示当前存在有效图像
	*/
	bool copyFrame(QImage* image, int64_t* frameStart100ns, int64_t* frameEnd100ns, uint64_t* frameSerial) const;

	/** 获取最近一次视频帧更新序号，界面层可据此避免重复复制大图
	*/
	uint64_t frameSerial() const;

	/** 获取当前视频原始尺寸，无视频时返回空尺寸
	*/
	QSize videoSize() const;

private:
	// 保护图像和尺寸，Core工作线程与GUI线程会同时访问
	mutable QMutex m_mutex;
	// 最近一帧ARGB图像，GUI线程按需复制
	QImage m_image;
	// 当前视频原始尺寸
	QSize m_videoSize;
	// 最近一帧起始时间，单位100纳秒
	int64_t m_frameStart100ns;
	// 最近一帧结束边界，单位100纳秒
	int64_t m_frameEnd100ns;
	// 最近一帧更新序号，每次新帧或关闭视频时递增
	uint64_t m_frameSerial;
};