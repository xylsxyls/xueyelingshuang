#pragma once
#include "LumaPlayerCoreMacro.h"
#include "LumaPlayerCoreTypes.h"

/** Core使用的视频渲染接口，接口由外部实现并通过LumaPlayerCore::setVideoRender注入，Core只保存指针不负责释放
*/
class LumaPlayerCoreAPI IVideoRender
{
public:
	/** 析构函数，保证通过接口指针释放外部派生类时行为正确
	*/
	virtual ~IVideoRender();

	/** 打开或重置视频输出格式
	@param [in] format Core输出的视频帧格式
	@return true表示外部渲染器已经准备好接收BGRA帧
	*/
	virtual bool openVideo(const LumaPlayerVideoFormat& format) = 0;

	/** 关闭视频输出并释放外部渲染器持有的显示资源
	*/
	virtual void closeVideo() = 0;

	/** 显示一帧视频画面
	@param [in] frame Core解码出的BGRA视频帧，回调返回后数据仍由Core管理
	@return true表示数据已经被外部渲染器接收
	*/
	virtual bool renderVideo(const LumaPlayerVideoFrame& frame) = 0;
};